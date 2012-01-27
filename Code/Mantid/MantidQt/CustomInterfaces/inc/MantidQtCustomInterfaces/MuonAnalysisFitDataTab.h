#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISFITDATATAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISFITDATATAB_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Ui
{
  class MuonAnalysis;
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{


/** 
This is a Helper class for MuonAnalysis. In particular this helper class deals
callbacks from the Plot Options tab.    

@author Robert Whitley, ISIS, RAL

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/

class MuonAnalysisFitDataTab : MantidQt::API::UserSubWindow
{
 Q_OBJECT

public:

  /// Constructor
  MuonAnalysisFitDataTab(Ui::MuonAnalysis& uiForm) : m_uiForm(uiForm) {}
  void init();

  void makeRawWorkspace(const std::string & wsName);
  void groupWorkspaces(const std::vector<std::string> & inputWorkspaces, const std::string & groupName);
  void groupFittedWorkspaces(QString workspaceName);
  
  QStringList getAllPlotDetails(const QString & workspace);

signals:

private:

  /// Initialize the layout
  virtual void initLayout() {};

  /// reference to MuonAnalysis form
  Ui::MuonAnalysis& m_uiForm;

private slots:
  
  void muonAnalysisHelpDataAnalysisClicked();

};


}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSISDITDATATAB_H_