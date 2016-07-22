#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMGGFORMATSCONVERTVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMGGFORMATSCONVERTVIEW_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <string>
#include <vector>

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
class IImggFormatsConvertView {

public:
  IImggFormatsConvertView() {}

  virtual ~IImggFormatsConvertView() {}

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
   * @param enableLoad (optional) whether to enable the loading of
   * each of the formats. If not given all of them will be enabled.
   *
   * @param enableSave (optional) whether to enable saving of
   * each of the formats. If not given all of them will be enabled.
   */
  virtual void setFormats(const std::vector<std::string> &fmts,
                          const std::vector<bool> &enableLoad = {},
                          const std::vector<bool> &enableSave = {}) = 0;

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
   * Load/SaveImage algorithm:
   * https://github.com/mantidproject/mantid/issues/6843
   *
   * @param inputName name of a readable image file(assuming Qt format
   * guess by header probing + extension)
   *
   * @param inputFormat image format to read
   *
   * @param outputName name of an output image file (assuming Qt
   * format guessing by suffix/extension)
   *
   * @param outputFormat image format to write
   */
  virtual void convert(const std::string &inputName,
                       const std::string &inputFormat,
                       const std::string &outputName,
                       const std::string &outputFormat) const = 0;

  /**
   * Write an image that has been loaded in a matrix workspace. As
   * with convert(), move out of here when we have a Load/SaveImage
   * algorithm.
   *
   * @param inWks workspace holding image data
   * @param outputName name for the output file
   * @param outFormat image format
   */
  virtual void writeImg(Mantid::API::MatrixWorkspace_sptr inWks,
                        const std::string &outputName,
                        const std::string &outFormat) const = 0;

  /**
   * Load an image in a matrix workspace. As with convert(), move out
   * of here when we have a Load/SaveImage algorithm.
   *
   * @param inputName name for the output file
   * @param inFormat image format
   *
   * @return a workspace with image data loaded from file
   */
  virtual Mantid::API::MatrixWorkspace_sptr
  loadImg(const std::string &inputName, const std::string &inFormat) const = 0;

  /**
   * Save this widget settings (when closing this widget).
   */
  virtual void saveSettings() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMGGFORMATSCONVERTVIEW_H_
