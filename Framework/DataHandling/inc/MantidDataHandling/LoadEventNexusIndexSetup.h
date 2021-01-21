// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
class MANTID_DATAHANDLING_DLL LoadEventNexusIndexSetup {
public:
  LoadEventNexusIndexSetup(API::MatrixWorkspace_const_sptr instrumentWorkspace, const int32_t min, const int32_t max,
                           const std::vector<int32_t> &range,
                           const Parallel::Communicator &communicator = Parallel::Communicator());

  std::pair<int32_t, int32_t> eventIDLimits() const;

  Indexing::IndexInfo makeIndexInfo();
  Indexing::IndexInfo makeIndexInfo(const std::vector<std::string> &bankNames);
  Indexing::IndexInfo
  makeIndexInfo(const std::pair<std::vector<int32_t>, std::vector<int32_t>> &spectrumDetectorMapping,
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
