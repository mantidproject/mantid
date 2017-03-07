#ifndef MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H

#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"

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

private:
  // Process selected rows
  void process() override;
  // Plotting
  void plotRow() override;
  void plotGroup() override;
  // Get the name of a post-processed workspace
  std::string getPostprocessedWorkspaceName(const GroupData &groupData,
                                            const std::string &prefix,
                                            double startTime, double stopTime);
  // Loads a group of runs
  bool loadGroup(const GroupData &group);
  // Process a group of runs which are event workspaces
  bool processGroupAsEventWS(int groupID, const GroupData &group,
                             const std::vector<double> &startTimes,
                             const std::vector<double> &stopTimes);
  // Process a group of runs which are not event workspaces
  bool processGroupAsNonEventWS(int groupID, const GroupData &group);
  // Parse time slicing from string
  void parseTimeSlicing(const std::string &timeSlicing,
                        std::vector<double> &startTimes,
                        std::vector<double> &stopTimes);
  // Load a run as event workspace
  bool loadEventRun(const std::string &runNo);
  // Load a run (non-event workspace)
  void loadNonEventRun(const std::string &runNo);
  // Take a slice from event workspace
  std::string takeSlice(const std::string &runNo, double startTime,
                        double stopTime);
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTER_H*/
