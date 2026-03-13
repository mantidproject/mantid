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
  // Create a direct beam workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr makeDirectBeamWS(Mantid::API::MatrixWorkspace_sptr inputWS);
  // Performs direct beam correction
  Mantid::API::MatrixWorkspace_sptr directBeamCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs transmission or algorithm correction
  Mantid::API::MatrixWorkspace_sptr transOrAlgCorrection(const Mantid::API::MatrixWorkspace_sptr &detectorWS,
                                                         const bool detectorWSReduced);
  // Performs background subtraction
  Mantid::API::MatrixWorkspace_sptr backgroundSubtraction(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs transmission corrections
  Mantid::API::MatrixWorkspace_sptr transmissionCorrection(const Mantid::API::MatrixWorkspace_sptr &detectorWS,
                                                           const bool detectorWSReduced);
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
                            const bool debug, int &step);
  // Create the output workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr makeIvsLam();
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
  bool m_convertUnits;          // convert the input workspace to lambda
  bool m_normaliseMonitors;     // normalise by monitors and direct beam
  bool m_normaliseTransmission; // transmission or algorithmic correction
  bool m_sum;                   // whether to do summation
  double m_theta0;              // horizon angle
  // groups of spectrum indices of the detectors of interest
  std::vector<std::vector<size_t>> m_detectorGroups;
  // Store the min/max wavelength we're interested in. These will be the
  // input Wavelength min/max if summing in lambda, or the projected
  // versions of these if summing in Q
  double m_wavelengthMin;
  double m_wavelengthMax;
  // True if partial bins should be included in the summation in Q
  bool m_partialBins;

  // Algorithm Tasks
  class AlgorithmTask {
  public:
    explicit AlgorithmTask(ReflectometryReductionOne3 *parent, const std::string &name)
        : m_parent(parent), m_name(name) {}
    void setDependantTask(std::string task, std::string output_name = "") {
      if (output_name.empty()) {
        // If no output name is provided, assume the whole task output is required
        m_dependantTasks[task] = {};
      } else {
        m_dependantTasks[task].push_back(output_name);
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
      executeImpl();
      checkExpectedOutputs();
    }
    std::vector<std::string> getExpectedOutputs() { return m_expectedOutputs; }
    void setExpectedOutputs(std::vector<std::string> expectedOutputs) { m_expectedOutputs = expectedOutputs; }
    std::string name() const { return m_name; }

  protected:
    void outputWorkspace(std::shared_ptr<MatrixWorkspace> ws, const std::string &outputName) {
      m_parent->m_algorithmTaskOutputs[m_name][outputName] = ws;
    }

  private:
    // map of dependant task name: dependant outputs
    std::unordered_map<std::string, std::vector<std::string>> m_dependantTasks;
    ReflectometryReductionOne3 *m_parent;
    std::string m_name;
    std::vector<std::string> m_expectedOutputs;

    virtual void executeImpl() = 0;

    void populateExpectedOutputs() {
      for (const auto &[taskName, outputs] : m_dependantTasks) {
        if (outputs.empty()) {
          // If no specific outputs are listed, populate with the whole task output
          if (!m_parent->m_algorithmTasks.contains(taskName))
            throw std::runtime_error("Dependant task " + taskName + " not found for task " + m_name +
                                     "could not populate expected outputs.");
          m_dependantTasks[taskName] = m_parent->m_algorithmTasks[taskName]->getExpectedOutputs();
        }
      }
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
      populateExpectedOutputs();
      std::vector<std::string> missingTasks;
      for (const auto &[taskName, outputs] : m_dependantTasks) {
        if (!m_parent->m_algorithmTaskOutputs.contains(taskName)) {
          missingTasks.push_back(taskName + ": ALL OUTPUTS");
        } else {
          for (const auto &output : outputs) {
            if (!m_parent->m_algorithmTaskOutputs[taskName].contains(output)) {
              missingTasks.push_back(taskName + ":" + output);
            }
          }
        }
      }
      return missingTasks;
    }
  };

  class TaskA : public AlgorithmTask {
  public:
    explicit TaskA(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskA") {
      setExpectedOutputs({"Output1", "Output2"});
    }
    void executeImpl() override {
      // Implementation of TaskA
      MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create("Workspace2D", 1, 5, 4);
      outputWorkspace(outputWS, "Output1");
      outputWorkspace(outputWS, "Output2");
    }
  };

  class TaskB : public AlgorithmTask {
  public:
    explicit TaskB(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskB") {
      setExpectedOutputs({"Output3"});
      setDependantTask("TaskA", "Output1");
    }
    void executeImpl() override {
      // Implementation of TaskB
      MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create("Workspace2D", 1, 5, 4);
      outputWorkspace(outputWS, "Output3");
    }
  };

  class TaskC : public AlgorithmTask {
  public:
    explicit TaskC(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskC") {
      setExpectedOutputs({"Output4"});
      setDependantTask("TaskB", "Output3");
    }

    void executeImpl() override {
      // Implementation of TaskC
      MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create("Workspace2D", 1, 5, 4);
      outputWorkspace(outputWS, "Output4");
    }
  };

  class TaskD : public AlgorithmTask {
  public:
    explicit TaskD(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, "TaskD") {
      setExpectedOutputs({"Output5"});
      setDependantTask("TaskC", "Output4");
    }
    void executeImpl() override {
      // Implementation of TaskD
      MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create("Workspace2D", 1, 5, 4);
      outputWorkspace(outputWS, "Output5");
    }
  };

  // map of task name: task
  std::map<std::string, std::shared_ptr<AlgorithmTask>> m_algorithmTasks;
  // map of task name: (map of output name: outputs)
  std::map<std::string, std::unordered_map<std::string, std::shared_ptr<MatrixWorkspace>>> m_algorithmTaskOutputs;
  void addAlgorithmTask(std::shared_ptr<AlgorithmTask> task) { m_algorithmTasks[task->name()] = task; }
  void addAlgorithmTasks(std::vector<std::shared_ptr<AlgorithmTask>> tasks) {
    for (const auto &task : tasks) {
      addAlgorithmTask(task);
    }
  }
};

} // namespace Reflectometry
} // namespace Mantid
