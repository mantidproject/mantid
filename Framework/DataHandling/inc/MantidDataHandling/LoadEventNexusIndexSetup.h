#ifndef MANTID_DATAHANDLING_LOADEVENTNEXUSINDEXSETUP_H_
#define MANTID_DATAHANDLING_LOADEVENTNEXUSINDEXSETUP_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidParallel/Communicator.h"

namespace Mantid {
namespace DataHandling {

/** Helper for LoadEventNexus dealing with setting up indices (spectrum numbers
  an detector ID mapping) for workspaces.

  Filters set via `min`, `max`, and `range` are used by LoadEventNexus for
  selecting from the `event_id` entry in Nexus files. This may either correspond
  to a spectrum number (ISIS) or a detector ID. Throughout this class IndexInfo
  is used for filtering and thus the spectrum number is set to the requested
  event_id ranges. The final returned IndexInfo will however have spectrum
  numbers that, in general, are not the event_ids (except for ISIS).

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_DATAHANDLING_DLL LoadEventNexusIndexSetup {
public:
  LoadEventNexusIndexSetup(
      API::MatrixWorkspace_const_sptr instrumentWorkspace, const int32_t min,
      const int32_t max, const std::vector<int32_t> range,
      const Parallel::Communicator &communicator = Parallel::Communicator());

  std::pair<int32_t, int32_t> eventIDLimits() const;

  Indexing::IndexInfo makeIndexInfo();
  Indexing::IndexInfo makeIndexInfo(const std::vector<std::string> &bankNames);
  Indexing::IndexInfo
  makeIndexInfo(const std::pair<std::vector<int32_t>, std::vector<int32_t>>
                    &spectrumDetectorMapping,
                const bool monitorsOnly);

private:
  Indexing::IndexInfo filterIndexInfo(const Indexing::IndexInfo &indexInfo);

  const API::MatrixWorkspace_const_sptr m_instrumentWorkspace;
  int32_t m_min;
  int32_t m_max;
  std::vector<int32_t> m_range;
  const Parallel::Communicator m_communicator;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADEVENTNEXUSINDEXSETUP_H_ */
