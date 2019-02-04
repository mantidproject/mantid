// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REBINALGORITHMDIALOGPROVIDER_H_
#define REBINALGORITHMDIALOGPROVIDER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/SlicingAlgorithmDialog.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
 This class coordinates the rebinning of a workspace and updates the pipeline
 and view to make the changes of the
 underlying workspace visible.

 @date 15/01/2015
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS
    RebinAlgorithmDialogProvider {
public:
  RebinAlgorithmDialogProvider(QWidget *parent);

  ~RebinAlgorithmDialogProvider();

  void showDialog(const std::string &inputWorkspace,
                  const std::string &outputWorkspace,
                  const std::string &algorithmType);

  static const size_t BinCutOffValue;

private:
  MantidQt::API::AlgorithmDialog *
  createDialog(const Mantid::API::IAlgorithm &algorithm,
               const std::string &inputWorkspace,
               const std::string &outputWorkspace,
               const std::string &algorithmType);

  void
  setAxisDimensions(MantidQt::MantidWidgets::SlicingAlgorithmDialog *dialog,
                    const std::string &inputWorkspace);

  Mantid::API::IMDEventWorkspace_sptr
  getWorkspace(const std::string &workspaceName);

  Mantid::API::IAlgorithm_sptr createAlgorithm(const std::string &algName,
                                               int version);

  Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>
      m_adsWorkspaceProvider;

  const QString m_lblInputWorkspace;
  const QString m_lblOutputWorkspace;
  QWidget *m_parent;
};

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif
