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

  /**
   * Display a warning to the user (normally as a pop-up).
   *
   * @param warn warning title, should be short and would normally be
   * shown as the title of the window or a big banner.
   *
   * @param description longer, free form and human readable
   * description of the issue.
   */
  virtual void userWarning(const std::string &warn,
                           const std::string &description) = 0;

  /**
   * Display a visible error message (normally as a pop-up).
   *
   * @param err error title, should be short and would normally be
   * shown as the title of the pop-up window or a big banner.
   *
   * @param description longer, free form description of the issue, as
   * user-understandable and detailed as posssible.
   */
  virtual void userError(const std::string &err,
                         const std::string &description) = 0;

  /**
   * Set the list of formats that should be shown to the user
   *
   * @param fmts list of formats (identified by a (short) name).
   *
   * @param enable (optional) whether to enable each of the
   * formats. If not given all of them will be enabled.
   */
  virtual void setFormats(const std::vector<std::string> &fmts,
                          const std::vector<bool> &enable = {}) = 0;

  /**
   * The input path to the files to convert, as selected by the user
   *
   * @return path as a string / validation is not done here
   */
  virtual std::string inputPath() const = 0;

  /**
   * Name of the format selected to pick input files.
   *
   * @return format name as a string
   */
  virtual std::string inputFormatName() const = 0;

  /**
   * The output / destination path for the converted files, as
   * selected by the user
   *
   * @return path as a string / validation is not done here
   */
  virtual std::string outputPath() const = 0;

  /**
   * Name of the output format selected.
   *
   * @return format name as a string
   */
  virtual std::string outputFormatName() const = 0;

  /**
   * User preference as to whether to compress the output images/data
   * files.
   *
   * @return compress or not
   */
  virtual bool compressHint() const = 0;

  /**
   * Maximum depth (subdirectories) to search for file from the input path.
   *
   * @return maximum depth set by the user
   */
  virtual size_t maxSearchDepth() const = 0;

  /**
   * Convert image (format A) to image (format B) when both formats
   * are only supported via Qt QImage and related classes.  TODO: This
   * should not be here. Move to presenter when we have the
   * Load/SaveImage algorithm.
   *
   * @param inputName name of a readable image file(assuming Qt format
   * guess by header probing + extension)
   *
   * @param outputName name of an output image file (assuming Qt
   * format guessing by suffix/extension)
   */
  virtual void convert(const std::string &inputName,
                       const std::string &outputName) const = 0;

  /**
   * Save this widget settings (when closing this widget).
   */
  virtual void saveSettings() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGINGFORMATSCONVERTVIEW_H_
