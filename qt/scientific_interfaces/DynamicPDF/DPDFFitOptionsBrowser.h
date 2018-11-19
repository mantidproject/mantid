// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
