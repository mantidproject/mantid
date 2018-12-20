#ifndef MANTID_MDALGORITHMS_MERGEMDFILES_H_
#define MANTID_MDALGORITHMS_MERGEMDFILES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"
#include <mutex>
#include <nexus/NeXusFile.hpp>

namespace Mantid {
namespace MDAlgorithms {

/** Algorithm to merge multiple MDEventWorkspaces from files that
 * obey a common box format.

  @author Janik Zikovsky
  @date 2011-08-16

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MergeMDFiles : public API::Algorithm {
public:
  MergeMDFiles();
  ~MergeMDFiles() override;

  /// Algorithm's name for identification
  const std::string name() const override { return "MergeMDFiles"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Merge multiple MDEventWorkspaces from files that obey a common box "
           "format.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"MergeMD"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Creation";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  void loadBoxData();

  void doExecByCloning(Mantid::API::IMDEventWorkspace_sptr ws,
                       const std::string &outputFile);

  void finalizeOutput(const std::string &outputFile);

  uint64_t loadEventsFromSubBoxes(API::IMDNode *TargetBox);

  // the class which flatten the box structure and deal with it
  DataObjects::MDBoxFlatTree m_BoxStruct;
  // the vector of box structures for contributing files components
  std::vector<DataObjects::MDBoxFlatTree> m_fileComponentsStructure;

protected:
  /// Set to true if the output is cloned of the first one
  // bool clonedFirst;
  void clearEventLoaders();

  /// number of workspace dimensions
  int m_nDims;
  /// string describes type of the event, stored in the workspaces.
  std::string m_MDEventType;

  /// if the workspace is indeed file-based
  bool m_fileBasedTargetWS;
  /// Files to load
  std::vector<std::string> m_Filenames;

  /// Vector of file handles to each input file //TODO unique?
  std::vector<API::IBoxControllerIO *> m_EventLoader;

  /// Output IMDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr m_OutIWS;

  /// # of events from ALL input files
  uint64_t m_totalEvents;

  /// # of events loaded from all tasks
  uint64_t m_totalLoaded;

  /// Mutex for file access
  std::mutex m_fileMutex;

  /// Mutex for modifying stats
  std::mutex m_statsMutex;

  /// Progress reporter
  std::unique_ptr<Mantid::API::Progress> m_progress = nullptr;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MERGEMDFILES_H_ */
