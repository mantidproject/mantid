// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_PARALLELEVENTLOADER_H_
#define MANTID_DATAHANDLING_PARALLELEVENTLOADER_H_

#include <string>
#include <vector>

#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataObjects {
class EventWorkspace;
}
namespace DataHandling {

/** Loader for event data from Nexus files with parallelism based on multiple
  processes (MPI or MultiProcessing) for performance. This class provides
  integration of the low level loader component Parallel::IO::EventLoader with
  higher level concepts such as DataObjects::EventWorkspace and the instrument.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_DATAHANDLING_DLL ParallelEventLoader {
public:
  static void loadMPI(DataObjects::EventWorkspace &ws,
                      const std::string &filename, const std::string &groupName,
                      const std::vector<std::string> &bankNames,
                      const bool eventIDIsSpectrumNumber);

  static void loadMultiProcess(DataObjects::EventWorkspace &ws,
                               const std::string &filename,
                               const std::string &groupName,
                               const std::vector<std::string> &bankNames,
                               const bool eventIDIsSpectrumNumber,
                               const bool precalcEvents);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_PARALLELEVENTLOADER_H_ */
