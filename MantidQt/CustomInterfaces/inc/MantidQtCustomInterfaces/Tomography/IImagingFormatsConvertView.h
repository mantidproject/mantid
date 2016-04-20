#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGINGFORMATSCONVERTVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGINGFORMATSCONVERTVIEW_H_

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
Widget to convert images and stacks of images between different image
formats.

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
class IImagingFormatsConvertView {

public:
  IImagingFormatsConvertView() {}

  virtual ~IImagingFormatsConvertView() {}

  virtual void userWarning(const std::string &err,
                           const std::string &description) = 0;

  virtual void userError(const std::string &err,
                         const std::string &description) = 0;

  void saveSettings() const;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGINGFORMATSCONVERTVIEW_H_
