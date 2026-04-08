// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/WorkspaceFactory.h" //remove this
#include "MantidReflectometry/ReflectometryWorkflowBase2.h"
#include <numeric>

namespace Mantid {
// Forward declaration
namespace API {
class SpectrumInfo;
}
namespace Geometry {
class ReferenceFrame;
}
namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
} // namespace HistogramData
namespace Reflectometry {

/** ReflectometryReductionOne3 : Reflectometry reduction of a single input TOF
 workspace to an IvsQ workspace. Version 3 of the algorithm.
 */
class MANTID_REFLECTOMETRY_DLL ReflectometryReductionOne3 : public ReflectometryWorkflowBase2 {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ReflectometryReductionOne"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 "
           "workspace. Performs monitor normalization and transmission "
           "corrections.";
  }
  /// Algorithm's version for identification.
  int version() const override { return 3; };
  const std::vector<std::string> seeAlso() const override { return {"ReflectometryReductionOneAuto"}; }
  /// Algorithm's category for identification.
  const std::string category() const override { return "Reflectometry"; };

private:
  /** Overridden Algorithm methods **/

  // Initialize the algorithm
  void init() override;
  // Execute the algorithm
  void exec() override;
  // Validate inputs
  std::map<std::string, std::string> validateInputs() override;
  // Set default names for output workspaces
  void setDefaultOutputWorkspaceNames();
  // Performs background subtraction
  Mantid::API::MatrixWorkspace_sptr backgroundSubtraction(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs transmission corrections
  std::pair<Mantid::API::MatrixWorkspace_sptr, Mantid::API::MatrixWorkspace_sptr>
  transmissionCorrection(const Mantid::API::MatrixWorkspace_sptr &detectorWS);
  // Performs transmission corrections using alternative correction algorithms
  Mantid::API::MatrixWorkspace_sptr algorithmicCorrection(const Mantid::API::MatrixWorkspace_sptr &detectorWS);
  // Performs monitor corrections
  Mantid::API::MatrixWorkspace_sptr monitorCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // convert to momentum transfer
  bool useDetectorAngleForQConversion(const MatrixWorkspace_sptr &ws) const;
  Mantid::API::MatrixWorkspace_sptr convertToQ(const Mantid::API::MatrixWorkspace_sptr &inputWS);
  // Get the twoTheta width of a given detector
  double getDetectorTwoThetaRange(const size_t spectrumIdx);
  // Utility function to create name for diagnostic workspaces
  std::string createDebugWorkspaceName(const std::string &inputName);
  // Utility function to output a diagnostic workspace to the ADS
  void outputDebugWorkspace(const API::MatrixWorkspace_sptr &ws, const std::string &wsName, const std::string &wsSuffix,
                            const int step);
  // Do the reduction by summation in Q
  Mantid::API::MatrixWorkspace_sptr sumInQ(const API::MatrixWorkspace_sptr &detectorWS);
  // Do the summation in Q for a single input value
  void sumInQProcessValue(const int inputIdx, const double twoTheta, const double bTwoTheta,
                          const HistogramData::HistogramX &inputX, const HistogramData::HistogramY &inputY,
                          const HistogramData::HistogramE &inputE, const std::vector<size_t> &detectors,
                          const size_t outSpecIdx, const API::MatrixWorkspace_sptr &IvsLam,
                          std::vector<double> &outputE);
  // Share counts to a projected value for summation in Q
  void sumInQShareCounts(const double inputCounts, const double inputErr, const double bLambda, const double lambdaMin,
                         const double lambdaMax, const size_t outSpecIdx, const API::MatrixWorkspace_sptr &IvsLam,
                         std::vector<double> &outputE);
  void findWavelengthMinMax(const API::MatrixWorkspace_sptr &inputWS);
  // Construct the output workspace
  void findIvsLamRange(const API::MatrixWorkspace_sptr &detectorWS, const std::vector<size_t> &detectors,
                       const double lambdaMin, const double lambdaMax, double &projectedMin, double &projectedMax);
  // Construct the output workspace
  Mantid::API::MatrixWorkspace_sptr constructIvsLamWS(const API::MatrixWorkspace_sptr &detectorWS);
  // Whether summation should be done in Q or the default lambda
  bool summingInQ() const;
  // Get projected coordinates onto twoThetaR
  void getProjectedLambdaRange(const double lambda, const double twoTheta, const double bLambda, const double bTwoTheta,
                               const std::vector<size_t> &detectors, double &lambdaTop, double &lambdaBot,
                               const bool outerCorners = true);
  // Check whether two spectrum maps match
  void verifySpectrumMaps(const API::MatrixWorkspace_const_sptr &ws1, const API::MatrixWorkspace_const_sptr &ws2);
  // Find and cache constants
  void findDetectorGroups();
  void findTheta0();
  // initialize algorithm members
  void initalizeMembers();

  // Accessors for detectors and theta and lambda values
  const std::vector<std::vector<size_t>> &detectorGroups() const { return m_detectorGroups; };
  double theta0() { return m_theta0; }
  double twoThetaR(const std::vector<size_t> &detectors);
  size_t twoThetaRDetectorIdx(const std::vector<size_t> &detectors);
  double wavelengthMin() { return m_wavelengthMin; };
  double wavelengthMax() { return m_wavelengthMax; };
  size_t findIvsLamRangeMinDetector(const std::vector<size_t> &detectors);
  size_t findIvsLamRangeMaxDetector(const std::vector<size_t> &detectors);
  double findIvsLamRangeMin(const Mantid::API::MatrixWorkspace_sptr &detectorWS, const std::vector<size_t> &detectors,
                            const double lambda);
  double findIvsLamRangeMax(const Mantid::API::MatrixWorkspace_sptr &detectorWS, const std::vector<size_t> &detectors,
                            const double lambda);

  API::MatrixWorkspace_sptr m_runWS;
  const API::SpectrumInfo *m_spectrumInfo;
  std::shared_ptr<const Mantid::Geometry::ReferenceFrame> m_refFrame;
  double m_theta0; // horizon angle
  // groups of spectrum indices of the detectors of interest
  std::vector<std::vector<size_t>> m_detectorGroups;
  // Store the min/max wavelength we're interested in. These will be the
  // input Wavelength min/max if summing in lambda, or the projected
  // versions of these if summing in Q
  double m_wavelengthMin;
  double m_wavelengthMax;
  // True if partial bins should be included in the summation in Q
  bool m_partialBins;
  // When a task sets wavelength min/max, flag this so it is not repeated.
  bool m_wavelengthMinMaxSet;

  // Algorithm Tasks
  class AlgorithmTask {
  public:
    explicit AlgorithmTask(ReflectometryReductionOne3 *parent, const std::string &name)
        : m_parent(parent), m_name(name), m_firstTaskFlag(false), m_activeDependantTaskSet(0) {
      addDependantTaskSet(); // Start with one dependant task set by default
    }
    size_t addDependantTaskSet() {
      m_dependantTasks.emplace_back();
      m_dependantOutputs.emplace_back();
      return m_dependantTasks.size() - 1;
    }
    void setDependantTask(const std::string &task, const std::string &output_name = "", const std::string &alias = "",
                          const size_t dependantTaskSet = 0) {
      if (dependantTaskSet >= m_dependantTasks.size())
        throw std::runtime_error("Dependant task set index " + std::to_string(dependantTaskSet) +
                                 " is out of range for task " + m_name);
      if (output_name.empty()) {
        // If no output name is provided, assume the whole task output is required
        // If no alias is provided, use the task name as the alias
        m_dependantTasks[dependantTaskSet][task] = {};
      } else {
        m_dependantTasks[dependantTaskSet][task].push_back({output_name, alias});
      }
    }
    void execute() {
      auto missingTasks = evaluateDependentTasks();
      if (!missingTasks.empty()) {
        throw std::runtime_error(
            "Cannot execute task " + m_name + " as the following dependent tasks outputs are not available: " +
            std::accumulate(std::next(missingTasks.begin()), missingTasks.end(), missingTasks.front(),
                            [](const std::string &a, const std::string &b) { return a + ", " + b; }));
      }
      activateTaskSet();
      m_parent->g_log.debug("Executing task: " + m_name + "\n");
      executeImpl();
      checkExpectedOutputs();
      m_parent->g_log.debug("Finished executing task: " + m_name + "\n");
      // ADD CLEAN UP OF MEMBER OJBECTS
    }
    std::vector<std::string> getExpectedOutputs() { return m_expectedOutputs; }
    void setExpectedOutputs(std::vector<std::string> expectedOutputs) { m_expectedOutputs = expectedOutputs; }
    std::string name() const { return m_name; }
    void initAsFirstTask(std::shared_ptr<MatrixWorkspace> inputWS) {
      addDependantTaskOutput("InputWorkspace", inputWS, 0);
      m_firstTaskFlag = true;
    }
    void setTaskExecutionOrder(const std::vector<std::string> *taskExecutionOrder) {
      m_taskExecutionOrder = taskExecutionOrder;
    }

    std::string getSelectedOutput() const { return m_selectedOutput; }

  protected:
    ReflectometryReductionOne3 *m_parent;
    void outputWorkspace(std::shared_ptr<MatrixWorkspace> ws, const std::string &outputName) {
      setSelectedOutput(outputName);
      m_parent->m_algorithmTaskOutputs[m_name][outputName] = ws;
    }

    std::shared_ptr<MatrixWorkspace> getDependantWorkspace(std::string outputAlias) {
      return m_dependantOutputs[m_activeDependantTaskSet][outputAlias];
    }

    void setSelectedOutput(const std::string &output, const bool overwrite = false) {
      // If overwrite is false, only set the selected output if it has not already been set
      if (!overwrite && !m_selectedOutput.empty())
        return;
      m_selectedOutput = output;
    }

  private:
    // vector of dependent task sets: map of dependant task name: dependant outputs (task name, alias pairs)
    std::vector<std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>> m_dependantTasks;
    std::string m_name;
    std::vector<std::string> m_expectedOutputs;
    std::vector<std::unordered_map<std::string, std::shared_ptr<MatrixWorkspace>>> m_dependantOutputs;
    bool m_firstTaskFlag;
    size_t m_activeDependantTaskSet;
    std::vector<int> m_fulfilledDependantTaskSets;
    const std::vector<std::string> *m_taskExecutionOrder = nullptr;
    std::string m_selectedOutput;

    virtual void executeImpl() = 0;

    bool populateDependantTasks(const size_t taskSetIndex) {
      for (auto &item : m_dependantTasks[taskSetIndex]) {
        const auto &taskName = item.first;
        auto &outputs = item.second;
        auto it = std::find_if(m_parent->m_stagedAlgorithmTasks.cbegin(), m_parent->m_stagedAlgorithmTasks.cend(),
                               [&taskName](std::shared_ptr<AlgorithmTask> task) { return task->name() == taskName; });
        if (it == m_parent->m_stagedAlgorithmTasks.cend())
          return false; // Task not found, this task set cannot be fulfilled
        if (outputs.empty()) {
          // If no specific outputs are listed, populate with the whole task output
          const auto &expectedOutputs = (*it)->getExpectedOutputs();
          for (const auto &output : expectedOutputs) {
            outputs.push_back({output, output});
          }
        }
      }
      return true;
    }

    void activateTaskSet() {
      if (m_fulfilledDependantTaskSets.size() == 1) {
        m_activeDependantTaskSet = m_fulfilledDependantTaskSets.front();
        return;
      } else if (m_fulfilledDependantTaskSets.size() == 0) {
        return;
      }
      // We have multiple fulfilled task sets, how do we choose between them?
      // Lets select based on the execution order of tasks.
      // The task set containing the task executed in closest proximity to this task wins.
      const auto myIt = std::find(m_taskExecutionOrder->cbegin(), m_taskExecutionOrder->cend(), m_name);
      const auto myIndex = std::distance(m_taskExecutionOrder->cbegin(), myIt);
      size_t closestTaskSet = 0;
      int closestDistance = std::numeric_limits<int>::max();
      for (auto taskSet : m_fulfilledDependantTaskSets) {
        for (auto &task : m_dependantTasks[taskSet]) {
          const auto &taskName = task.first;
          auto it = std::find(m_taskExecutionOrder->cbegin(), m_taskExecutionOrder->cend(), taskName);
          std::size_t index = std::distance(m_taskExecutionOrder->cbegin(), it);
          int distance = (int)myIndex - (int)index;
          // Do not consider tasks that occur after the current task
          if ((distance < closestDistance) && distance > 0) {
            closestDistance = distance;
            closestTaskSet = taskSet;
          }
        }
      }
      m_activeDependantTaskSet = closestTaskSet;
    }

    // after execution, check that expected outputs from this task are present in m_algorithmTaskOutputs
    void checkExpectedOutputs() {
      if (!m_parent->m_algorithmTaskOutputs.contains(m_name))
        throw std::runtime_error("No output from task " + m_name + " found after task execution");

      std::vector<std::string> missingOutput;
      for (const auto &output : m_expectedOutputs) {
        if (!m_parent->m_algorithmTaskOutputs[m_name].contains(output))
          missingOutput.push_back(output);
      }
      if (!missingOutput.empty()) {
        throw std::runtime_error(
            "Expected outputs from task " + m_name + " not found after task execution: " +
            std::accumulate(std::next(missingOutput.begin()), missingOutput.end(), missingOutput.front(),
                            [](const std::string &a, const std::string &b) { return a + ", " + b; }));
      }
    }

    // check if output from dependant tasks is available in m_algorithmTaskOutputs
    // this could be supplied by dependant tasks, or manually setting algorithm properties
    std::vector<std::string> evaluateDependentTasks() {
      // If this is the first task, we expect the dependant outputs to be set as algorithm properties rather than
      // outputs from other tasks
      // TODO: Check that required input properties are provided. Currently we just take the input workspace
      if (m_firstTaskFlag)
        return {};
      std::vector<std::string> missingTasksAll;
      // Loop through each task set
      for (auto i = 0; i < m_dependantTasks.size(); ++i) {
        // if task set is unfulfillable due to missing tasks
        std::vector<std::string> missingTasks;
        if (!populateDependantTasks(i)) {
          // TODO: print out required tasks
          missingTasks.push_back("Task set " + std::to_string(i) + " unfulfillable as required tasks not staged.");
        } else {
          for (const auto &[taskName, outputs] : m_dependantTasks[i]) {
            if (!m_parent->m_algorithmTaskOutputs.contains(taskName)) {
              missingTasks.push_back("Task set: " + std::to_string(i) + " Task name: " + taskName + ": ALL OUTPUTS");
            } else {
              for (const auto &output : outputs) {
                if (!m_parent->m_algorithmTaskOutputs[taskName].contains(output.first)) {
                  missingTasks.push_back("Task set: " + std::to_string(i) + " Task name: " + taskName + ": " +
                                         output.first);
                } else {
                  // populate dependent outputs for use in the task execution
                  addDependantTaskOutput(output.second, m_parent->m_algorithmTaskOutputs[taskName][output.first], i);
                }
              }
            }
          }
        }
        // If we have found a task set with all outputs available, add this to fulfilled sets
        // otherwise, add missing tasks to the list of missing tasks for all sets
        if (missingTasks.empty()) {
          m_fulfilledDependantTaskSets.push_back(i);
        } else {
          missingTasksAll.insert(missingTasksAll.end(), missingTasks.begin(), missingTasks.end());
        }
      }
      return (m_fulfilledDependantTaskSets.empty() ? missingTasksAll : std::vector<std::string>{});
    }

    void addDependantTaskOutput(const std::string &outputName, std::shared_ptr<MatrixWorkspace> ws,
                                const size_t taskSetIndex) {
      m_dependantOutputs[taskSetIndex][outputName] = ws;
    }
  };

  class TaskBackgroundSubtraction : public AlgorithmTask {
  public:
    explicit TaskBackgroundSubtraction(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, "TaskBackgroundSubtraction") {
      setExpectedOutputs({"BackgroundSubtractedWorkspace"});
      setDependantTask("TaskExtractROI", "ExtractedROIWorkspace", "InputWorkspace");
    }
    void executeImpl() override;
  };

  class TaskConvertToWavelength : public AlgorithmTask {
  public:
    explicit TaskConvertToWavelength(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, "TaskConvertToWavelength") {
      setExpectedOutputs({"ConvertedWorkspaceWavelength"});
      setDependantTask("TaskBackgroundSubtraction", "BackgroundSubtractedWorkspace", "InputWorkspace");
      const auto taskSet = addDependantTaskSet();
      setDependantTask("TaskSumDetectors", "SummedWorkspace", "InputWorkspace", taskSet);
    }
    void executeImpl() override;
  };

  class TaskNormalizeByMonitor : public AlgorithmTask {
  public:
    explicit TaskNormalizeByMonitor(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, "TaskNormalizeByMonitor") {
      setExpectedOutputs({"MonitorCorrectedWorkspace"});
      setDependantTask("TaskSumDetectors", "SummedWorkspace", "InputWorkspace");
      const auto taskSet = addDependantTaskSet();
      setDependantTask("TaskConvertToWavelength", "ConvertedWorkspaceWavelength", "InputWorkspace", taskSet);
    }
    void executeImpl() override;
  };

  class TaskNormalizeByTransmission : public AlgorithmTask {
  public:
    explicit TaskNormalizeByTransmission(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, "TaskNormalizeByTransmission") {
      setExpectedOutputs({"TransmissionCorrectedWorkspace"});
      setDependantTask("TaskNormalizeByMonitor", "MonitorCorrectedWorkspace", "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask("TaskConvertToWavelength", "ConvertedWorkspaceWavelength", "InputWorkspace", taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask("TaskCropWavelength", "CroppedWorkspace", "InputWorkspace", taskSet2);
    }
    void executeImpl() override;
  };

  class TaskNormalizeByAlgorithm : public AlgorithmTask {
  public:
    explicit TaskNormalizeByAlgorithm(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, "TaskNormalizeByAlgorithm") {
      setExpectedOutputs({"AlgorithmCorrectedWorkspace"});
      setDependantTask("TaskNormalizeByMonitor", "MonitorCorrectedWorkspace", "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask("TaskConvertToWavelength", "ConvertedWorkspaceWavelength", "InputWorkspace", taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask("TaskCropWavelength", "CroppedWorkspace", "InputWorkspace", taskSet2);
    }
    void executeImpl() override;
  };

  class TaskExtractROI : public AlgorithmTask {
  public:
    explicit TaskExtractROI(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskExtractROI") {
      setExpectedOutputs({"ExtractedROIWorkspace"});
    }
    void executeImpl() override;
  };

  class TaskSumDetectors : public AlgorithmTask {
  public:
    explicit TaskSumDetectors(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskSumDetectors") {
      setExpectedOutputs({"SummedWorkspace"});
      setDependantTask("TaskConvertToWavelength", "ConvertedWorkspaceWavelength", "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask("TaskBackgroundSubtraction", "BackgroundSubtractedWorkspace", "InputWorkspace", taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask("TaskExtractROI", "ExtractedROIWorkspace", "InputWorkspace", taskSet2);
    }
    void executeImpl() override;
  };

  class TaskSumDetectorsInQ : public AlgorithmTask {
  public:
    explicit TaskSumDetectorsInQ(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskSumDetectorsInQ") {
      setExpectedOutputs({"QSummedWorkspace"});
      setDependantTask("TaskNormalizeByMonitor", "MonitorCorrectedWorkspace", "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask("TaskNormalizeByTransmission", "TransmissionCorrectedWorkspace", "InputWorkspace", taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask("TaskConvertToWavelength", "ConvertedWorkspaceWavelength", "InputWorkspace", taskSet2);
    }
    void executeImpl() override;
  };

  class TaskCropWavelength : public AlgorithmTask {
  public:
    explicit TaskCropWavelength(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskCropWavelength") {
      setExpectedOutputs({"CroppedWorkspace"});
      setDependantTask("TaskNormalizeByMonitor", "MonitorCorrectedWorkspace", "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask("TaskSumDetectorsInQ", "QSummedWorkspace", "InputWorkspace", taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask("TaskConvertToWavelength", "ConvertedWorkspaceWavelength", "InputWorkspace", taskSet2);
    }
    void executeImpl() override;
  };

  class TaskConvertToQ : public AlgorithmTask {
  public:
    explicit TaskConvertToQ(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskConvertToQ") {
      setExpectedOutputs({"ConvertedWorkspaceQ"});
      setDependantTask("TaskCropWavelength", "CroppedWorkspace", "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask("TaskNormalizeByTransmission", "TransmissionCorrectedWorkspace", "InputWorkspace", taskSet1);
    }
    void executeImpl() override;
  };

  std::vector<std::shared_ptr<AlgorithmTask>> m_AlgorithmTasks{
      std::make_shared<TaskExtractROI>(this),          std::make_shared<TaskBackgroundSubtraction>(this),
      std::make_shared<TaskConvertToWavelength>(this), std::make_shared<TaskSumDetectors>(this),
      std::make_shared<TaskNormalizeByMonitor>(this),  std::make_shared<TaskNormalizeByTransmission>(this),
      std::make_shared<TaskCropWavelength>(this),      std::make_shared<TaskConvertToQ>(this),
      std::make_shared<TaskSumDetectorsInQ>(this),     std::make_shared<TaskNormalizeByAlgorithm>(this)};
  std::vector<std::shared_ptr<AlgorithmTask>> m_stagedAlgorithmTasks;
  // map of task name: (map of output name: outputs)
  std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<MatrixWorkspace>>>
      m_algorithmTaskOutputs;
  void stageAlgorithmTasks(std::vector<std::shared_ptr<AlgorithmTask>> tasks) {
    if (tasks.empty())
      return;
    // for first task in sequence, feed in the input workspace
    API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    tasks[0]->initAsFirstTask(inputWS);
    m_stagedAlgorithmTasks = tasks;
  }
  std::vector<std::string> configureAlgorithmTasks();
  std::vector<std::string> constructTaskExecutionOrder();
};

} // namespace Reflectometry
} // namespace Mantid
