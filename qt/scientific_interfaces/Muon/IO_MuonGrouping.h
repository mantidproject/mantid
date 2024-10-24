// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/

class MANTIDQT_MUONINTERFACE_DLL MuonGroupingHelper {
public:
  /// Constructor
  MuonGroupingHelper(Ui::MuonAnalysis &uiForm) : m_uiForm(uiForm) {};

  /// Saves grouping to the XML file specified
  static void saveGroupingToXML(const Mantid::API::Grouping &grouping, const std::string &filename);

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
