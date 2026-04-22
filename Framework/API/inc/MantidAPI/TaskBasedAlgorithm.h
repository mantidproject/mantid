// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "AnalysisDataService.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MatrixWorkspace.h"

namespace Mantid::API {

template <class T> class TaskBasedAlgorithm : virtual public API::DataProcessorAlgorithm {
protected:
  class AlgorithmTask {
  public:
    explicit AlgorithmTask(T *parent, const std::string &name)
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
    const std::vector<std::string> &getExpectedOutputs() const { return m_expectedOutputs; }
    void setExpectedOutputs(const std::vector<std::string> &expectedOutputs) { m_expectedOutputs = expectedOutputs; }
    const std::string &name() const { return m_name; }
    void initAsFirstTask(std::shared_ptr<MatrixWorkspace> inputWS) {
      addDependantTaskOutput("InputWorkspace", inputWS, 0);
      m_firstTaskFlag = true;
    }
    void setTaskExecutionOrder(const std::vector<std::string> *taskExecutionOrder) {
      m_taskExecutionOrder = taskExecutionOrder;
    }
    const std::string &getSelectedOutput() const { return m_selectedOutput; }

  protected:
    T *m_parent;
    void outputWorkspace(std::shared_ptr<MatrixWorkspace> ws, const std::string &outputName) {
      setSelectedOutput(outputName);
      m_parent->m_algorithmTaskOutputs[m_name][outputName] = ws;
    }

    std::shared_ptr<MatrixWorkspace> getDependantWorkspace(const std::string &outputAlias) {
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
    std::vector<size_t> m_fulfilledDependantTaskSets;
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
          std::transform(expectedOutputs.begin(), expectedOutputs.end(), std::back_inserter(outputs),
                         [](const auto &output) { return std::pair<std::string, std::string>{output, output}; });
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
        for (const auto &task : m_dependantTasks[taskSet]) {
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
      const auto &taskOutputs = m_parent->m_algorithmTaskOutputs[m_name];
      std::copy_if(m_expectedOutputs.begin(), m_expectedOutputs.end(), std::back_inserter(missingOutput),
                   [&taskOutputs](const auto &output) { return !taskOutputs.contains(output); });
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
      for (size_t i = 0; i < m_dependantTasks.size(); ++i) {
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

  void configureAlgorithmTasks() {
    m_taskExecutionOrder =
        isDefault("TaskExecutionOrder") ? constructTaskExecutionOrder() : getProperty("TaskExecutionOrder");
    std::vector<std::shared_ptr<AlgorithmTask>> tasksToStage(m_taskExecutionOrder.size());
    for (auto &task : m_AlgorithmTasks) {
      task->setTaskExecutionOrder(&m_taskExecutionOrder);
      auto it = std::find(m_taskExecutionOrder.begin(), m_taskExecutionOrder.end(), task->name());
      if (it != m_taskExecutionOrder.end()) {
        std::size_t index = std::distance(m_taskExecutionOrder.begin(), it);
        tasksToStage[index] = task;
      }
    }
    stageAlgorithmTasks(tasksToStage);
  }

  void execTasks(const std::string &diagWorkspacePrefix = "") {
    configureAlgorithmTasks();
    int step = 0;
    for (size_t i = 0; i < m_stagedAlgorithmTasks.size(); ++i) {
      const auto &task = m_stagedAlgorithmTasks[i];
      task->execute();
      if (!diagWorkspacePrefix.empty()) {
        const auto &taskOutput = m_algorithmTaskOutputs.at(task->name());
        for (const auto &output : taskOutput) {
          const auto &outputName = output.first;
          outputDebugWorkspace(output.second, diagWorkspacePrefix, "_" + outputName, step);
        }
        step++;
      }
      // Output the selected output of the last task
      if (i == m_stagedAlgorithmTasks.size() - 1)
        setProperty("OutputWorkspace", m_algorithmTaskOutputs.at(task->name()).at(task->getSelectedOutput()));
    }
  }

  void outputDebugWorkspace(const MatrixWorkspace_sptr &ws, const std::string &wsName, const std::string &wsSuffix,
                            const int step) {
    // Clone the workspace because otherwise we can end up outputting the same
    // workspace twice with different names, which is confusing.
    MatrixWorkspace_sptr cloneWS = ws->clone();
    AnalysisDataService::Instance().addOrReplace(wsName + "_" + std::to_string(step) + wsSuffix, cloneWS);
  }

  template <typename... TaskTypes>
  void initTaskBasedAlgorithm(const std::vector<std::string> &defaultTaskExecutionOrder = {}) {
    static_assert((std::is_base_of_v<AlgorithmTask, TaskTypes> && ...), "All TaskTypes must derive from AlgorithmTask");
    m_AlgorithmTasks.clear();
    m_AlgorithmTasks.reserve(sizeof...(TaskTypes));
    auto *parent = static_cast<T *>(this);
    (m_AlgorithmTasks.emplace_back(std::make_shared<TaskTypes>(parent)), ...);
    declareProperty("TaskExecutionOrder", defaultTaskExecutionOrder, "The tasks to execute, in execution order.");
  }

  // Returns default TaskExecutionOrder property, if manipulation is necessary
  // provide override
  virtual std::vector<std::string> constructTaskExecutionOrder() {
    std::vector<std::string> teo = getProperty("TaskExecutionOrder");
    return teo;
  };

  std::vector<std::shared_ptr<AlgorithmTask>> m_AlgorithmTasks;
  std::vector<std::string> m_taskExecutionOrder;
};
} // namespace Mantid::API
