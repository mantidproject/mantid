#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_

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

/** 
This is a collection of helper functions for MuonAnalysis.h. In particular dealing with
loading and saving of xml grouping files into the interface.    

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

/// save XML grouping file
void saveGroupingTabletoXML(Ui::MuonAnalysis& m_uiForm, const std::string& filename);

/// load XML grouping file
void loadGroupingXMLtoTable(Ui::MuonAnalysis& m_uiForm, const std::string& filename);

/// create 'map' relating group number to row number in group table
void whichGroupToWhichRow(Ui::MuonAnalysis& m_uiForm, std::vector<int>& groupToRow);

/// create 'map' relating pair number to row number in pair table
void whichPairToWhichRow(Ui::MuonAnalysis& m_uiForm, std::vector<int>& pairToRow);

/// Set Group / Group Pair name
void setGroupGroupPair(Ui::MuonAnalysis& m_uiForm, const std::string& name);

}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
