#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGFORMATS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGFORMATS_H_

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/**
File formats used to store imaging data (image file formats and
generic imaging formats such as NXTomo
http://download.nexusformat.org/sphinx/classes/applications/NXtomo.html).

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
namespace ImggFormats {
enum Format { FITS = 0, TIFF = 1, PNG = 2, JPG = 3, NXTomo = 4 };

std::string shortName(Format fmt);

std::vector<std::string> fileExtension(Format fmt);

std::string fileExtension(const std::string &format);

bool isFileExtension(const std::string &extension,
                     const std::string &shortName);

std::string description(Format fmt);

} // namespace ImggFormats

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGFORMATS_H_
