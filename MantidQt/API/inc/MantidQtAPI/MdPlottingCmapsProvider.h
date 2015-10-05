#ifndef MDPLOTTINGCMAPSPROVIDER_H_
#define MDPLOTTINGCMAPSPROVIDER_H_

#include "DllOption.h"
#include <QString>
#include <vector>

class QStringList;

namespace MantidQt
{
  namespace API
  {
    /**
      *
      This helper class allows for reading and processing the names of the available MD color map files

      @date 7/1/2015

      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

      This file is part of Mantid.

      Mantid is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 3 of the License, or
      (at your option) any later version.

      Mantid is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>.

      File change history is stored at: <https://github.com/mantidproject/mantid>
      Code Documentation is available at: <http://doxygen.mantidproject.org>
      */

    class EXPORT_OPT_MANTIDQT_API MdPlottingCmapsProvider
    {
      public:
        MdPlottingCmapsProvider();

        ~MdPlottingCmapsProvider();

        /**
         * Get the name of all available color maps for general MD plotting.
         * @param colorMapNames Reference to a list with the names of the color maps.
         * @param colorMapFiles Reference to a corresponding list with the file paths for the color maps.
         */
        void getColorMapsForMdPlotting(QStringList& colorMapNames, QStringList& colorMapFiles);

        /**
         * Get the name of all available color maps for the VSI (at least the ones stored in files).
         * @param colorMapNames Reference to a list with the names of the color maps.
         */
        void getColorMapsForVSI(QStringList& colorMapNames);

      private:
        /** 
         * Gets all files from directory of a given file type.
         * @param colorMapNames Reference to a list of color map names.
         * @param colorMapFiles Reference to a list of file paths for the corresponding color maps.
         * @param colorMapDirectory Directory where the color maps are stored.
         * @param fileType suffix of the desired files.
         */
        void appendAllFileNamesForFileType(QStringList& colorMapNames, QStringList& colorMapFiles, QString colorMapDirectory, QString fileType);

         /** 
         * Gets all the color map names 
         * @param colorMapNames Reference to a list of color map names.
         * @param fullFilePath File path to the xml files with the color map definitions.
         */
        void appendVSIColorMaps(QStringList& colorMapNames,  QString fullFilePath);

        /**
         * Compare the colormap names of the Slice Viewer and the VSI and extract all indicees of the list of Slice Viewer color maps
         * which also exist in the list of the VSI color maps.
         * @param colorMapNamesSliceViewer A list of color maps of the Slice Viewer.
         * @param colorMapNamesVsi A list of color maps for the VSI.
         * @returns A vector of indices for the slice viewer list.
         */
        std::vector<int> getSliceViewerIndicesForCommonColorMaps(QStringList colorMapNamesSliceViewer,QStringList colorMapNamesVsi);
    };
  }
}

#endif 
