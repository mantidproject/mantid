#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ui_MuonAnalysis.h"
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

  /// save XML grouping file
  void saveGroupingTabletoXML(Ui::MuonAnalysis& m_uiForm, const std::string& filename);

  /// load XML grouping file
  void loadGroupingXMLtoTable(Ui::MuonAnalysis& m_uiForm, const std::string& filename);

}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
