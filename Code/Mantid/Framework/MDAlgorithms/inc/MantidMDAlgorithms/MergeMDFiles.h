#ifndef MANTID_MDALGORITHMS_MERGEMDFILES_H_
#define MANTID_MDALGORITHMS_MERGEMDFILES_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
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
  ~MergeMDFiles();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "MergeMDFiles"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Merge multiple MDEventWorkspaces from files that obey a common box "
           "format.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

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
  uint64_t totalEvents;

  /// # of events loaded from all tasks
  uint64_t totalLoaded;

  /// Mutex for file access
  Kernel::Mutex fileMutex;

  /// Mutex for modifying stats
  Kernel::Mutex statsMutex;

  /// Progress reporter
  Mantid::API::Progress *prog;

  /// Set to true if the output is cloned of the first one
  // bool clonedFirst;
  void clearEventLoaders();
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MERGEMDFILES_H_ */
