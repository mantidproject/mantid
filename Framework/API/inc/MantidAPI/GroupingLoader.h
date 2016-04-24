#ifndef MANTID_API_GROUPINGLOADER_H_
#define MANTID_API_GROUPINGLOADER_H_

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
  Grouping(ITableWorkspace_sptr table);

  /// Convert to grouping table
  ITableWorkspace_sptr toTable() const;
};

/** GroupingLoader : Loads instrument grouping from IDF file

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL GroupingLoader {
public:
  explicit GroupingLoader(Geometry::Instrument_const_sptr instrument);
  GroupingLoader(Geometry::Instrument_const_sptr instrument,
                 const std::string &mainFieldDirection);
  virtual ~GroupingLoader();
  /// Load the grouping from the instrument's IDF
  boost::shared_ptr<Grouping> getGroupingFromIDF() const;
  /// Loads grouping from the XML file specified
  static void loadGroupingFromXML(const std::string &filename,
                                  Grouping &grouping);
  /// Returns a "dummy" grouping of a single group with all the detectors in it
  boost::shared_ptr<Grouping> getDummyGrouping();

private:
  /// Instrument to load grouping from
  const Geometry::Instrument_const_sptr m_instrument;
  /// Orientation of instrument (e.g. for MUSR)
  const std::string m_mainFieldDirection;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_GROUPINGLOADER_H_ */