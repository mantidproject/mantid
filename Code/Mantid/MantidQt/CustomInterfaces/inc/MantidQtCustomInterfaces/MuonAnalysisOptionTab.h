#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtMantidWidgets/MWDiag.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QTableWidget>

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

@author Anders Markvardsen, ISIS, RAL

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class MuonAnalysisOptionTab : public QWidget
{
 Q_OBJECT
public:
  MuonAnalysisOptionTab(Ui::MuonAnalysis& uiForm, const QString& group) : m_uiForm(uiForm), m_settingsGroup(group) {}
  void initLayout();

public slots:

  ///////////// Plot options ////////////
  ///
  void runTimeComboBox(int index);
  ///
  void runTimeAxisStartAtInput();
  ///
  void runTimeAxisFinishAtInput();
  ///
  void runyAxisMinimumInput();
  ///
  void runyAxisMaximumInput();
  ///
  void runShowErrorBars(bool state);
  ///
  void runyAxisAutoscale(bool state);

private:

  ///
  Ui::MuonAnalysis& m_uiForm;

  /// group defaults are saved to
  const QString& m_settingsGroup;
};

}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSISOPTIONTAB_H_
