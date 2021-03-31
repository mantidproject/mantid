// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace API {

/// Structure to represent grouping information
class MANTID_API_DLL Grouping {
public:
  std::vector<std::string> groupNames;
  std::vector<std::string> groups; // Range strings, e.g. "1-32"

  std::vector<std::string> pairNames;
  std::vector<std::pair<size_t, size_t>> pairs; // Pairs of group ids
  std::vector<double> pairAlphas;

  std::string description;
  std::string defaultName; // Not storing id because can be either group or pair

  /// Default constructor
  Grouping() = default;

  /// Destructor
  ~Grouping();

  /// Construct a Grouping from a table
  Grouping(const ITableWorkspace_sptr &table);

  /// Convert to grouping table
  ITableWorkspace_sptr toTable() const;
};

/** GroupingLoader : Loads instrument grouping from IDF file
 */
class MANTID_API_DLL GroupingLoader {
public:
  explicit GroupingLoader(Geometry::Instrument_const_sptr instrument);
  GroupingLoader(Geometry::Instrument_const_sptr instrument, const std::string &mainFieldDirection);
  virtual ~GroupingLoader();
  /// Load the grouping from the instrument's IDF
  std::shared_ptr<Grouping> getGroupingFromIDF() const;
  /// Loads grouping from the XML file specified
  static void loadGroupingFromXML(const std::string &filename, Grouping &grouping);
  /// Returns a "dummy" grouping of a single group with all the detectors in it
  std::shared_ptr<Grouping> getDummyGrouping();

private:
  /// Instrument to load grouping from
  const Geometry::Instrument_const_sptr m_instrument;
  /// Orientation of instrument (e.g. for MUSR)
  const std::string m_mainFieldDirection;
};

} // namespace API
} // namespace Mantid
