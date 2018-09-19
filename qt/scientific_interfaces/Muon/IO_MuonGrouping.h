#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_

//----------------------
// Includes
//----------------------
#include "DllConfig.h"
#include "MantidAPI/GroupingLoader.h"
#include "ui_MuonAnalysis.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {

/**
This is a collection of helper functions for MuonAnalysis.h. In particular
dealing with grouping files in the interface.

@author Anders Markvardsen, ISIS, RAL

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTIDQT_MUONINTERFACE_DLL MuonGroupingHelper {
public:
  /// Constructor
  MuonGroupingHelper(Ui::MuonAnalysis &uiForm) : m_uiForm(uiForm){};

  /// Saves grouping to the XML file specified
  static void saveGroupingToXML(const Mantid::API::Grouping &grouping,
                                const std::string &filename);

  /// Parses information from the grouping table and saves to Grouping struct
  Mantid::API::Grouping parseGroupingTable() const;

  /// Fills in the grouping table using information from provided Grouping
  /// struct
  int fillGroupingTable(const Mantid::API::Grouping &grouping);

  /// create 'map' relating group number to row number in group table
  std::vector<int> whichGroupToWhichRow() const;

  /// create 'map' relating pair number to row number in pair table
  std::vector<int> whichPairToWhichRow() const;

  /// Get index of Group / Group Pair name
  int getGroupGroupPairIndex(const std::string &name);

private:
  /// Reference to UI
  Ui::MuonAnalysis &m_uiForm;
};
} // namespace Muon
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_IO_GROUPING_H_
