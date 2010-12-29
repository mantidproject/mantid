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

  /// create 'map' relating group number to row number in group table
  void whichGroupToWhichRow(Ui::MuonAnalysis& m_uiForm, std::vector<int>& groupToRow);

  /// create 'map' relating pair number to row number in pair table
  void whichPairToWhichRow(Ui::MuonAnalysis& m_uiForm, std::vector<int>& pairToRow);
}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
