#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/DllConfig.h"

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

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

using namespace Mantid;
using namespace Mantid::API;

/// Structure to represent grouping information for Muon Analysis
struct Grouping {
  std::vector<std::string> groupNames;
  std::vector<std::string> groups; // Range strings, e.g. "1-32"

  std::vector<std::string> pairNames;
  std::vector<std::pair<size_t, size_t> > pairs; // Pairs of group ids
  std::vector<double> pairAlphas;

  std::string description;
  std::string defaultName; // Not storing id because can be either group or pair
};

/// Saves grouping to the XML file specified
void MANTIDQT_CUSTOMINTERFACES_DLL saveGroupingToXML(const Grouping& grouping, 
  const std::string& filename);

/// Loads grouping from the XML file specified
void MANTIDQT_CUSTOMINTERFACES_DLL loadGroupingFromXML(const std::string& filename, 
  Grouping& grouping);

/// Parses information from the grouping table and saves to Grouping struct
void MANTIDQT_CUSTOMINTERFACES_DLL parseGroupingTable(const Ui::MuonAnalysis& form, 
  Grouping& grouping);

/// Fills in the grouping table using information from provided Grouping struct
void MANTIDQT_CUSTOMINTERFACES_DLL fillGroupingTable(const Grouping& grouping, 
  Ui::MuonAnalysis& form);

/// Groups the workspace according to grouping provided
MatrixWorkspace_sptr MANTIDQT_CUSTOMINTERFACES_DLL groupWorkspace(MatrixWorkspace_const_sptr ws, 
  const Grouping& g);

/// create 'map' relating group number to row number in group table
void MANTIDQT_CUSTOMINTERFACES_DLL whichGroupToWhichRow(const Ui::MuonAnalysis& m_uiForm, 
  std::vector<int>& groupToRow);

/// create 'map' relating pair number to row number in pair table
void MANTIDQT_CUSTOMINTERFACES_DLL whichPairToWhichRow(const Ui::MuonAnalysis& m_uiForm, 
  std::vector<int>& pairToRow);

/// Set Group / Group Pair name
void MANTIDQT_CUSTOMINTERFACES_DLL setGroupGroupPair(Ui::MuonAnalysis& m_uiForm, 
  const std::string& name);

/// Convert a grouping table to a grouping struct
boost::shared_ptr<Grouping> MANTIDQT_CUSTOMINTERFACES_DLL tableToGrouping(ITableWorkspace_sptr table);

/// Converts a grouping information to a grouping table
ITableWorkspace_sptr MANTIDQT_CUSTOMINTERFACES_DLL groupingToTable(boost::shared_ptr<Grouping> grouping);

/// Returns a "dummy" grouping which a single group with all the detectors in it
boost::shared_ptr<Grouping> getDummyGrouping(Instrument_const_sptr instrument);

/// Attempts to load a grouping information referenced by IDF
boost::shared_ptr<Grouping> getGroupingFromIDF(Instrument_const_sptr instrument,
                                               const std::string& mainFieldDirection);


}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
