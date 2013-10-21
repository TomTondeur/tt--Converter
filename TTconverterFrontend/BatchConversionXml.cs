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
using System.Xml;
using System.Xml.Schema;
using System.Xml.Serialization;

namespace TTconverterFrontend
{
    public class BatchConversionXml : IXmlSerializable
    {
        public Window ParentWindow { get; set; }
        public string OutputDir { get; set; }
        public Dictionary<string, FbxFileDesc> FileDescs { get; set; }

        //Should just return null
        public XmlSchema GetSchema()
        {
            return null;
        }

        //Attempt to read batch.xml to load previous settings
        //If anything goes wrong, just start without any preloaded settings
        public void ReadXml(XmlReader reader)
        {
            try
            {
                FileDescs = new Dictionary<string, FbxFileDesc>();

                reader.ReadToFollowing("BatchConversion");

                reader.ReadToDescendant("Output");
                OutputDir = reader.ReadElementContentAsString();

                while (reader.LocalName == "FbxFile")
                {
                    var newFile = new FbxFileDesc();

                    reader.ReadToDescendant("Filename");
                    var newFilename = reader.ReadElementContentAsString();

                    if (reader.LocalName != "CollisionGeneration")
                        reader.ReadToNextSibling("CollisionGeneration");

                    newFile.CollisionType = reader.ReadElementContentAsString();

                    while (reader.LocalName == "AnimClip")
                    {
                        var newClip = new AnimationClipDesc();

                        reader.ReadToDescendant("Name");
                        var newClipName = reader.ReadElementContentAsString();

                        if (reader.LocalName != "Keyframes")
                        reader.ReadToNextSibling("Keyframes");

                        newClip.BeginFrame = double.Parse(reader.GetAttribute("Begin"));
                        newClip.EndFrame = double.Parse(reader.GetAttribute("End"));
                        newClip.Fps = double.Parse(reader.GetAttribute("FPS"));

                        reader.Read();
                        reader.ReadEndElement();

                        newFile.AnimationClips.Add(newClipName, newClip);
                    }

                    reader.ReadEndElement();
                    FileDescs.Add(newFilename, newFile);
                }
            }
            catch (Exception)
            {
                MessageBox.Show(ParentWindow, "Unable to read batch.xml, starting with a clean slate...",
                                "Error", MessageBoxButton.OK,
                                MessageBoxImage.Error);
                FileDescs = new Dictionary<string, FbxFileDesc>();
                OutputDir = "";
            }
        }

        //Write an xml file containing all the info on the files entered in the program
        public void WriteXml(XmlWriter writer)
        {
            writer.WriteStartDocument();
            writer.WriteWhitespace("\n\n");

            writer.WriteStartElement("BatchConversion");
            writer.WriteWhitespace("\n\n");
            
            writer.WriteElementString("Output", OutputDir);
            writer.WriteWhitespace("\n");
            
            foreach (var fileDesc in FileDescs)
            {
                if (!File.Exists(fileDesc.Key))
                {
                    MessageBox.Show(ParentWindow, string.Format("Unable to find file {0}. \nThis file will be skipped.",fileDesc.Key),
                                    "Error", MessageBoxButton.OK,
                                    MessageBoxImage.Error);
                    continue;
                }
                writer.WriteWhitespace("\n");

                writer.WriteStartElement("FbxFile");
                writer.WriteWhitespace("\n\t");

                writer.WriteElementString("Filename", fileDesc.Key);
                writer.WriteWhitespace("\n\t");

                writer.WriteElementString("CollisionGeneration", fileDesc.Value.CollisionType);
                writer.WriteWhitespace("\n");

                foreach (var animClip in fileDesc.Value.AnimationClips)
                {
                    writer.WriteWhitespace("\n\t");

                    writer.WriteStartElement("AnimClip");
                    writer.WriteWhitespace("\n\t\t");

                    writer.WriteElementString("Name", animClip.Key);
                    writer.WriteWhitespace("\n\t\t");

                    writer.WriteStartElement("Keyframes");
                    
                    writer.WriteAttributeString("Begin", animClip.Value.BeginFrame.ToString(CultureInfo.InvariantCulture));
                    writer.WriteAttributeString("End", animClip.Value.EndFrame.ToString(CultureInfo.InvariantCulture));
                    writer.WriteAttributeString("FPS", animClip.Value.Fps.ToString(CultureInfo.InvariantCulture));

                    writer.WriteEndElement(); //Keyframes
                    writer.WriteWhitespace("\n\t");

                    writer.WriteEndElement(); //AnimClip
                    writer.WriteWhitespace("\n");
                }

                writer.WriteEndElement(); // FbxFile
                writer.WriteWhitespace("\n");
            }

            writer.WriteWhitespace("\n");
            writer.WriteEndElement(); // BatchConversion

            writer.WriteEndDocument();
        }
    }
}
