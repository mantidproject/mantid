// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidMDAlgorithms/DllConfig.h"
#include <mutex>

namespace Mantid {
namespace MDAlgorithms {

/** Algorithm to merge multiple MDEventWorkspaces from files that
 * obey a common box format.

  @author Janik Zikovsky
  @date 2011-08-16
*/
class MANTID_MDALGORITHMS_DLL MergeMDFiles final : public API::Algorithm {
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
  const std::vector<std::string> seeAlso() const override { return {"MergeMD"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Creation"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  void loadBoxData();

  void doExecByCloning(const Mantid::API::IMDEventWorkspace_sptr &ws, const std::string &outputFile);

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
