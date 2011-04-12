#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISHELPER_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISHELPER_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"
#include "MantidQtCustomInterfaces/MuonAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtMantidWidgets/MWDiag.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QTableWidget>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{

  /// Add Greek letter to label from code 
  void createMicroSecondsLabels(Ui::MuonAnalysis& m_uiForm);

  // auto save various gui values
  void autoSave(Ui::MuonAnalysis& m_uiForm);

}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSISHELPER_H_
