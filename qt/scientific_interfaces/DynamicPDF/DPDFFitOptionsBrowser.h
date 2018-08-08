#ifndef MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DPDFFITOPTIONSBROWSER_H_
#define MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DPDFFITOPTIONSBROWSER_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
// Mantid headers from other projects
#include "DllConfig.h"
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
// 3rd party library headers
// System headers

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/** Class DPDFFitOptionsBrowser implements QtPropertyBrowser to display
 and set properties of Fit algorithm (excluding Function and Workspace).
 Customizes class FitOptionsBrowser.

  @date 2016-23-22

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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
class MANTIDQT_DYNAMICPDF_DLL DPDFFitOptionsBrowser
    : public MantidQt::MantidWidgets::FitOptionsBrowser {

  Q_OBJECT

public:
  DPDFFitOptionsBrowser(QWidget *parent = nullptr);

private:
  void createAdditionalProperties();
  void customizeBrowser();
  /// Starting fitting range
  QtProperty *m_startX;
  /// Ending fitting range
  QtProperty *m_endX;

}; // class DPDFFitOptionsBrowser

} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DPDFFITOPTIONSBROWSER_H_
