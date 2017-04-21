#ifndef MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H

#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets;

/** @class ReflDataProcessorPresenter

ReflDataProcessorPresenter is a presenter class that inherits from
GenericDataProcessorPresenter and re-implements some methods

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL ReflDataProcessorPresenter
    : public GenericDataProcessorPresenter {
public:
  // Constructor
  ReflDataProcessorPresenter(
      const DataProcessorWhiteList &whitelist,
      const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
          preprocessMap,
      const DataProcessorProcessingAlgorithm &processor,
      const DataProcessorPostprocessingAlgorithm &postprocessor,
      const std::map<std::string, std::string> &postprocessMap =
          std::map<std::string, std::string>(),
      const std::string &loader = "Load");
  ~ReflDataProcessorPresenter() override;

  // The following methods are public for testing purposes only
  // Add entry for the number of slices for a row in a group
  void addNumSlicesEntry(int groupID, int rowID, size_t numSlices);
  // Add entry for the number of slices for all rows in a group
  void addNumGroupSlicesEntry(int groupID, size_t numSlices);

private:
  // Process selected rows
  void process() override;
  // Plotting
  void plotRow() override;
  void plotGroup() override;
  // Loads a run from disk
  std::string loadRun(const std::string &run, const std::string &instrument,
                      const std::string &prefix, const std::string &loader,
                      bool &runFound);
  // Get the name of a post-processed workspace
  std::string getPostprocessedWorkspaceName(const GroupData &groupData,
                                            const std::string &prefix,
                                            size_t index);
  // Loads a group of runs
  bool loadGroup(const GroupData &group);
  // Process a group of runs which are event workspaces
  bool processGroupAsEventWS(int groupID, const GroupData &group,
                             const std::string &timeSlicingType,
                             const std::string &timeSlicingValues);
  // Process a group of runs which are not event workspaces
  bool processGroupAsNonEventWS(int groupID, const GroupData &group);

  // Parse uniform / uniform even time slicing from input string
  void parseUniform(const std::string &timeSlicing,
                    const std::string &slicingType, const std::string &wsName,
                    std::vector<double> &startTimes,
                    std::vector<double> &stopTimes);
  // Parse custom time slicing from input string
  void parseCustom(const std::string &timeSlicing,
                   std::vector<double> &startTimes,
                   std::vector<double> &stopTimes);

  // Load a run as event workspace
  bool loadEventRun(const std::string &runNo);
  // Load a run (non-event workspace)
  void loadNonEventRun(const std::string &runNo);
  // Take a slice from event workspace
  std::string takeSlice(const std::string &runNo, size_t sliceIndex,
                        double startTime, double stopTime);
  // Asks user if they wish to proceed if a type of workspace exists in the ADS
  bool proceedIfWSTypeInADS(const MantidQt::MantidWidgets::TreeData &data,
                            const bool findEventWS);

  std::map<int, std::map<int, size_t>> m_numSlicesMap;
  std::map<int, size_t> m_numGroupSlicesMap;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H*/
