// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_MuonAnalysis.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Ui {
class MuonAnalysis;
}

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {

/**
This is a Helper class for MuonAnalysis. In particular this helper class deals
callbacks from the Plot Options tab.

@author Robert Whitley, ISIS, RAL
*/

class MuonAnalysisFitDataTab : MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  /// Constructor.
  explicit MuonAnalysisFitDataTab(Ui::MuonAnalysis &uiForm) : m_uiForm(uiForm) {}
  /// Initialise.
  void init();
  /// Copy the given raw workspace and keep for later.
  void makeRawWorkspace(const std::string &wsName);

signals:

private:
  /// Initialize the layout.
  void initLayout() override{};
  /// Reference to MuonAnalysis form.
  Ui::MuonAnalysis &m_uiForm;

private slots:
  /// Open up the wiki help.
  void muonAnalysisHelpDataAnalysisClicked();
  /// Group all the workspaces made after a fitting.
  void groupFittedWorkspaces(const QString &workspaceName);
};
} // namespace Muon
} // namespace CustomInterfaces
} // namespace MantidQt
