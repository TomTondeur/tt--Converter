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

using System.Collections.Generic;

namespace TTconverterFrontend
{
    //Contains all information needed to extract an animation clip
    public class AnimationClipDesc
    {
        public double BeginFrame { get; set; }
        public double EndFrame { get; set; }
        public double Fps { get; set; }
    }

    //Contains all information needed to convert an fbx file
    public class FbxFileDesc
    {
        public string CollisionType { get; set; }
        public Dictionary<string, AnimationClipDesc> AnimationClips { get; set; } 

        public FbxFileDesc()
        {
            AnimationClips = new Dictionary<string, AnimationClipDesc>();
            CollisionType = "";
        }
    }
}
