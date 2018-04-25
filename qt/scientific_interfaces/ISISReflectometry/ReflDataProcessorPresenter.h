#ifndef MANTID_ISISREFLECTOMETRY_REFLDATAPROCESSORPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLDATAPROCESSORPRESENTER_H

#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeManager.h"
#include "MantidAPI/IEventWorkspace_fwd.h"

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Utility class to store information about time slicing
 */
class TimeSlicingInfo {
public:
  TimeSlicingInfo() = delete;
  TimeSlicingInfo(QString type, QString values);

  size_t numberOfSlices() const;
  double sliceDuration() const;
  double startTime(const size_t i) { return m_startTimes[i]; }
  double stopTime(const size_t i) { return m_stopTimes[i]; }
  QString values() { return m_values; }
  QString logFilter() { return m_logFilter; }

  bool hasSlicing() const { return m_enableSlicing && !m_values.isEmpty(); }
  bool isCustom() const { return m_type == "Custom"; }
  bool isLogValue() const { return m_type == "LogValue"; }
  bool isUniform() const { return m_type == "Uniform"; }
  bool isUniformEven() const { return m_type == "UniformEven"; }

  void addSlice(const double startTime, const double stopTime);
  void clearSlices();
  void parseUniform();
  void parseUniformEven();
  void parseCustom();
  void parseLogValue();

private:
  // the slicing type specified by the user
  QString m_type;
  // the slicing values specified by the user
  QString m_values;
  // whether time slicing is enabled or not
  bool m_enableSlicing;
  // the number of slices (where this is constant for all slices)
  size_t m_constNumberOfSlices;
  // the duration of the slices (where this is constant for all slices)
  double m_constSliceDuration;
  // The start and stop times for all slices for all rows in all groups. If
  // using non-even slicing then different runs may have different numbers of
  // slices. These vectors will contain ALL slices. It is assumed the first n
  // number of common slices will be the same for all runs and the difference
  // will be that later slices do not exist for some runs.
  std::vector<double> m_startTimes;
  std::vector<double> m_stopTimes;
  QString m_logFilter;
};

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
  // Add entry for the number of slices for all rows in a group
  void addNumGroupSlicesEntry(int groupID, size_t numSlices);
  // end reduction
  void endReduction(const bool success) override;

  void
  completedRowReductionSuccessfully(GroupData const &groupData,
                                    std::string const &workspaceName) override;
  void completedGroupReductionSuccessfully(
      GroupData const &groupData, std::string const &workspaceName) override;

protected slots:
  void threadFinished(const int exitCode) override;

private:
  // Get the processing options for this row
  OptionsMap getProcessingOptions(RowData_sptr data) override;
  // Process given items
  void process(TreeData itemsToProcess) override;
  // Plotting
  void plotRow() override;
  void plotGroup() override;
  // Loads a run from disk
  QString loadRun(const QString &run, const QString &instrument,
                  const QString &prefix, const QString &loader, bool &runFound);
  // Loads a group of runs
  bool loadGroup(const GroupData &group);
  // Get the property names of workspaces affected by slicing
  std::vector<QString> getSlicedWorkspacePropertyNames() const;
  // Reduce a row that is an event workspace
  bool reduceRowAsEventWS(RowData_sptr rowData, TimeSlicingInfo &slicing);
  // Process a group of runs which are event workspaces
  bool processGroupAsEventWS(int groupID, const GroupData &group,
                             TimeSlicingInfo &slicingInfo);
  // Process a group of runs which are not event workspaces
  bool processGroupAsNonEventWS(int groupID, GroupData &group);

  // Parse uniform / uniform even time slicing from input string
  void parseUniform(TimeSlicingInfo &slicing, const QString &wsName);
  bool workspaceExists(QString const &workspaceName) const;

  // Load a run as event workspace
  bool loadEventRun(const QString &runNo);
  // Load a run (non-event workspace)
  void loadNonEventRun(const QString &runNo);

  // Take a slice from event workspace
  QString takeSlice(const QString &runNo, TimeSlicingInfo &slicing,
                    size_t sliceIndex);

  Mantid::API::IEventWorkspace_sptr
  retrieveWorkspaceOrCritical(QString const &name) const;

  Mantid::API::IEventWorkspace_sptr
  retrieveWorkspace(QString const &name) const;

  // Asks user if they wish to proceed if a type of workspace exists in the ADS
  bool proceedIfWSTypeInADS(
      const MantidQt::MantidWidgets::DataProcessor::TreeData &data,
      const bool findEventWS);

  std::map<int, size_t> m_numGroupSlicesMap;
};
}
}
#endif /*MANTID_ISISREFLECTOMETRY_REFLDATAPROCESSORPRESENTER_H*/
