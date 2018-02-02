#ifndef MANTID_ISISREFLECTOMETRY_REFLDATAPROCESSORPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLDATAPROCESSORPRESENTER_H

#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeManager.h"
#include "MantidAPI/IEventWorkspace_fwd.h"

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets::DataProcessor;

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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflDataProcessorPresenter
    : public GenericDataProcessorPresenter {
public:
  // Constructor
  ReflDataProcessorPresenter(
      const WhiteList &whitelist,
      const std::map<QString, PreprocessingAlgorithm> &preprocessMap,
      const ProcessingAlgorithm &processor,
      const PostprocessingAlgorithm &postprocessor, int group,
      const std::map<QString, QString> &postprocessMap =
          std::map<QString, QString>(),
      const QString &loader = "Load");
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
  QString loadRun(const QString &run, const QString &instrument,
                  const QString &prefix, const QString &loader, bool &runFound);
  // Get the name of a post-processed workspace
  QString getPostprocessedWorkspaceName(const GroupData &groupData,
                                        const QString &prefix, size_t index);
  // Loads a group of runs
  bool loadGroup(const GroupData &group);
  // Process a group of runs which are event workspaces
  bool processGroupAsEventWS(int groupID, const GroupData &group,
                             const QString &timeSlicingType,
                             const QString &timeSlicingValues);
  // Process a group of runs which are not event workspaces
  bool processGroupAsNonEventWS(int groupID, GroupData &group);

  // Parse uniform / uniform even time slicing from input string
  void parseUniform(const QString &timeSlicing, const QString &slicingType,
                    const QString &wsName, std::vector<double> &startTimes,
                    std::vector<double> &stopTimes);
  // Parse custom time slicing from input string
  void parseCustom(const QString &timeSlicing, std::vector<double> &startTimes,
                   std::vector<double> &stopTimes);
  // Parse log value slicing and filter from input string
  void parseLogValue(const QString &inputStr, QString &logFilter,
                     std::vector<double> &minValues,
                     std::vector<double> &maxValues);
  bool workspaceExists(QString const &workspaceName) const;

  // Load a run as event workspace
  bool loadEventRun(const QString &runNo);
  // Load a run (non-event workspace)
  void loadNonEventRun(const QString &runNo);

  // Take a slice from event workspace
  QString takeSlice(const QString &runNo, size_t sliceIndex, double startTime,
                    double stopTime, const QString &logFilter = "");

  Mantid::API::IEventWorkspace_sptr
  retrieveWorkspaceOrCritical(QString const &name) const;

  Mantid::API::IEventWorkspace_sptr
  retrieveWorkspace(QString const &name) const;

  // Asks user if they wish to proceed if a type of workspace exists in the ADS
  bool proceedIfWSTypeInADS(
      const MantidQt::MantidWidgets::DataProcessor::TreeData &data,
      const bool findEventWS);

  std::map<int, std::map<int, size_t>> m_numSlicesMap;
  std::map<int, size_t> m_numGroupSlicesMap;
};
}
}
#endif /*MANTID_ISISREFLECTOMETRY_REFLDATAPROCESSORPRESENTER_H*/
