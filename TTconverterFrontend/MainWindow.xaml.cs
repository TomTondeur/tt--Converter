// Copyright © 2013 Tom Tondeur
// 
// This file is part of tt::Converter.
// 
// tt::Converter is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// tt::Converter is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with tt::Converter.  If not, see <http://www.gnu.org/licenses/>.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Forms;
using System.Windows.Interop;
using System.Xml;
using IWin32Window = System.Windows.Forms.IWin32Window;
using MessageBox = System.Windows.MessageBox;
using OpenFileDialog = Microsoft.Win32.OpenFileDialog;

namespace TTconverterFrontend
{
    //Extension method so we can pass a Window to WinForms (folder browser)
    public class Wpf32Window : IWin32Window
    {
        public IntPtr Handle { get; private set; }

        public Wpf32Window(Window wpfWindow)
        {
            Handle = new WindowInteropHelper(wpfWindow).Handle;
        }
    }

    public static class CommonDialogExtensions
    {
        public static DialogResult ShowDialog
                                   (this CommonDialog dialog,
                                         Window parent)
        {
            return dialog.ShowDialog(new Wpf32Window(parent));
        }
    }

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //All files added to the batch
        private Dictionary<string, FbxFileDesc> _allFiles = new Dictionary<string, FbxFileDesc>();
        //File currently being edited
        private FbxFileDesc _selectedFile;

        public MainWindow()
        {
            InitializeComponent();
        }

        //Browse for a new fbx file
        private void BtnFileBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog
                {
                    Filter = "FBX files (.FBX)|*.fbx|All Files|*.*",
                    FilterIndex = 1,
                    Multiselect = false
                };

            var isOk = dialog.ShowDialog(this);
            if (isOk == true)
                TxtFileSelected.Text = dialog.FileName;
        }

        //Get the filedesc referenced in the textbox, or create a new one if it doesn't exist
        //Reset the other UI items
        private void TxtFileSelected_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            if (!_allFiles.TryGetValue(TxtFileSelected.Text, out _selectedFile))
                _selectedFile = new FbxFileDesc();

            switch (_selectedFile.CollisionType)
            {
                case "Concave":
                    RadColConcave.IsChecked = true;
                    break;
                case "Convex":
                    RadColConvex.IsChecked = true;
                    break;
                case "None":
                    RadColNone.IsChecked = true;
                    break;
                default:
                    RadColConcave.IsChecked = false;
                    RadColConvex.IsChecked = false;
                    RadColNone.IsChecked = false;
                    break;
            }
            ListBoxAnimClips.Items.Clear();
            foreach (var clip in _selectedFile.AnimationClips)
            {
                ListBoxAnimClips.Items.Add(clip.Key);
            }
            TxtAnimClipSelected.Text = "";
            TxtAnimClipBegin.Text = "";
            TxtAnimClipEnd.Text = "";
            TxtAnimClipFps.Text = "";
        }

        //Add the currently selected file to the batch
        private void BtnAddFile_Click(object sender, RoutedEventArgs e)
        {
            if (RadColConcave.IsChecked == true)
                _selectedFile.CollisionType = "Concave";
            else if (RadColConvex.IsChecked == true)
                _selectedFile.CollisionType = "Convex";
            else if (RadColNone.IsChecked == true)
                _selectedFile.CollisionType = "None";
            else
            {
                MessageBox.Show(this, "Please select the collision to generate.", "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
                return;
            }

            try
            {
                _allFiles.Add(TxtFileSelected.Text, _selectedFile);
                ListBoxAllFiles.Items.Add(TxtFileSelected.Text);
            }
            catch (ArgumentException exceptionObj)
            {
                MessageBox.Show(this, exceptionObj.Message, "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
            }
        }

        //Remove the currently selected file from the batch
        private void BtnRemoveFile_Click(object sender, RoutedEventArgs e)
        {
            _allFiles.Remove(TxtFileSelected.Text);
            ListBoxAllFiles.Items.Remove(TxtFileSelected.Text);
        }

        //Add an animation clip to the currently selected file
        private void BtnAddAnimClip_Click(object sender, RoutedEventArgs e)
        {
            if (_selectedFile == null)
            {
                MessageBox.Show(this, "Cannot add animation clips without an input file.", "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
                return;
            }
            try
            {
                var newClip = new AnimationClipDesc
                    {
                        BeginFrame = double.Parse(TxtAnimClipBegin.Text),
                        EndFrame = double.Parse(TxtAnimClipEnd.Text),
                        Fps = double.Parse(TxtAnimClipFps.Text)
                    };

                if(newClip.BeginFrame > newClip.EndFrame || newClip.Fps < 0)
                    throw new FormatException();

                _selectedFile.AnimationClips.Add(TxtAnimClipSelected.Text, newClip);
                ListBoxAnimClips.Items.Add(TxtAnimClipSelected.Text);
                TxtAnimClipSelected.Text = "";
                TxtAnimClipBegin.Text = "";
                TxtAnimClipEnd.Text = "";
                TxtAnimClipSelected.Focus();
            }
            catch (ArgumentException)
            {
                MessageBox.Show(this, "An animation clip with this name already exists, please choose a different name.",
                                "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
            }
            catch (FormatException)
            {
                MessageBox.Show(this, "Please ensure you have entered valid numbers for the Begin, End and FPS fields.",
                                "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
            }
        }

        //Remove an animation clip from the currently selected file
        private void BtnRemoveAnimClip_Click(object sender, RoutedEventArgs e)
        {
            _selectedFile.AnimationClips.Remove(TxtAnimClipSelected.Text);
            ListBoxAnimClips.Items.Remove(TxtAnimClipSelected.Text);
        }

        //Select an animation clip
        private void ListBoxAnimClips_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            TxtAnimClipSelected.Text = (string)ListBoxAnimClips.SelectedItem;

            if (ListBoxAnimClips.SelectedItem == null)
            {
                TxtAnimClipBegin.Text = "";
                TxtAnimClipEnd.Text = "";
                TxtAnimClipFps.Text = "";
                return;
            }
            var clip = _selectedFile.AnimationClips[TxtAnimClipSelected.Text];
            TxtAnimClipBegin.Text = clip.BeginFrame.ToString(CultureInfo.InvariantCulture);
            TxtAnimClipEnd.Text = clip.EndFrame.ToString(CultureInfo.InvariantCulture);
            TxtAnimClipFps.Text = clip.Fps.ToString(CultureInfo.InvariantCulture);
        }

        //Validate input for begin keyframe
        private void TxtAnimClipBegin_LostFocus(object sender, RoutedEventArgs e)
        {
            AnimationClipDesc clip;
            if (!_selectedFile.AnimationClips.TryGetValue(TxtAnimClipSelected.Text, out clip))
                return;
            try
            {
                clip.BeginFrame = double.Parse(TxtAnimClipBegin.Text);
                double end;
                if (double.TryParse(TxtAnimClipEnd.Text, out end) && end < clip.BeginFrame)
                    throw new FormatException();
            }
            catch (FormatException)
            {
                MessageBox.Show(this, "Begin contains invalid input, this change will not be saved.",
                                "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
            }
        }

        //Validate input for end keyframe
        private void TxtAnimClipEnd_LostFocus(object sender, RoutedEventArgs e)
        {
            AnimationClipDesc clip;
            if (!_selectedFile.AnimationClips.TryGetValue(TxtAnimClipSelected.Text, out clip))
                return;
            try
            {
                clip.EndFrame = double.Parse(TxtAnimClipEnd.Text);
                double begin;
                if(double.TryParse(TxtAnimClipBegin.Text, out begin) && begin > clip.EndFrame)
                    throw new FormatException();
            }
            catch (FormatException)
            {
                MessageBox.Show(this, "End contains invalid input, this change will not be saved.",
                                "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
            }
        }

        //Validate input for anim clip FPS
        private void TxtAnimClipFps_LostFocus(object sender, RoutedEventArgs e)
        {
            AnimationClipDesc clip;
            if (!_selectedFile.AnimationClips.TryGetValue(TxtAnimClipSelected.Text, out clip))
                return;
            try
            {
                clip.Fps = double.Parse(TxtAnimClipFps.Text);

                if (clip.Fps < 0)
                    throw new FormatException();
            }
            catch (FormatException)
            {
                MessageBox.Show(this, "FPS contains invalid input, this change will not be saved.",
                                "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
            }
        }

        //Select a file
        private void ListBoxAllFiles_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            TxtFileSelected.Text = (string)ListBoxAllFiles.SelectedItem;
        }

        //Browse for an output directory
        private void BtnOutDirBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new FolderBrowserDialog();

            var isOk = dialog.ShowDialog(this);
            if (isOk == System.Windows.Forms.DialogResult.OK)
                TxtOutDir.Text = dialog.SelectedPath + "\\";
        }

        //Build batch.xml and launch the backend
        private void BtnConvert_Click(object sender, RoutedEventArgs e)
        {
            if (_allFiles.Count == 0)
            {
                MessageBox.Show(this, "Add .fbx files to serialize first.",
                                "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
                return;
            }
            if (TxtOutDir.Text == "")
            {
                MessageBox.Show(this, "Please provide an output directory.",
                                "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
                return;
            }
            //Save settings to batch.xml
            using (var fstream = new FileStream("batch.xml", FileMode.Create))
            {
                using (var writer = XmlWriter.Create(fstream))
                {
                    var xmlFile = new BatchConversionXml
                        {
                            FileDescs = _allFiles,
                            OutputDir = TxtOutDir.Text,
                            ParentWindow = this
                        };
                    xmlFile.WriteXml(writer);
                }
            }

            //Run the backend
            System.Diagnostics.Process.Start("TTconverterBackend.exe");
        }

        //Load settings from batch.xml
        private void ClientWindow_Initialized(object sender, EventArgs e)
        {
            using (var fstream = new FileStream("batch.xml", FileMode.Open, FileAccess.Read))
            {
                var xmlSettings = new XmlReaderSettings {IgnoreWhitespace = true};
                using (var reader = XmlReader.Create(fstream, xmlSettings))
                {
                    var xmlFile = new BatchConversionXml {ParentWindow = this};
                    xmlFile.ReadXml(reader);

                    _allFiles = xmlFile.FileDescs;

                    foreach (var file in _allFiles)
                        ListBoxAllFiles.Items.Add(file.Key);

                    TxtOutDir.Text = xmlFile.OutputDir;
                }
            }
        }

    }
}
