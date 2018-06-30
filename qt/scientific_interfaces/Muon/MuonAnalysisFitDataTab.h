#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISFITDATATAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISFITDATATAB_H_

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

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MuonAnalysisFitDataTab : MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  /// Constructor.
  explicit MuonAnalysisFitDataTab(Ui::MuonAnalysis &uiForm)
      : m_uiForm(uiForm) {}
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

#endif // MANTIDQTCUSTOMINTERFACES_MUONANALYSISFITDATATAB_H_
