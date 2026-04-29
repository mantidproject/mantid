// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/TaskBasedAlgorithm.h"
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

struct Tasks {
  struct NormalizeByMonitor {
    static constexpr const char *name = "TaskNormalizeByMonitor";
    struct Outputs {
      static constexpr const char *MonitorCorrectedWorkspace = "MonitorCorrectedWorkspace";
    };
  };
  struct NormalizeByTransmission {
    static constexpr const char *name = "TaskNormalizeByTransmission";
    struct Outputs {
      static constexpr const char *TransmissionCorrectedWorkspace = "TransmissionCorrectedWorkspace";
      static constexpr const char *TransmissionWorkspace = "TransmissionWorkspace";
    };
  };
  struct NormalizeByAlgorithm {
    static constexpr const char *name = "TaskNormalizeByAlgorithm";
    struct Outputs {
      static constexpr const char *AlgorithmCorrectedWorkspace = "AlgorithmCorrectedWorkspace";
    };
  };
  struct SumDetectorsInQ {
    static constexpr const char *name = "TaskSumDetectorsInQ";
    struct Outputs {
      static constexpr const char *QSummedWorkspace = "QSummedWorkspace";
    };
  };
  struct CropWavelength {
    static constexpr const char *name = "TaskCropWavelength";
    struct Outputs {
      static constexpr const char *CroppedWorkspace = "CroppedWorkspace";
    };
  };
  struct ConvertToQ {
    static constexpr const char *name = "TaskConvertToQ";
    struct Outputs {
      static constexpr const char *ConvertedWorkspaceQ = "ConvertedWorkspaceQ";
    };
  };
  struct ConvertToWavelength {
    static constexpr const char *name = "TaskConvertToWavelength";
    struct Outputs {
      static constexpr const char *ConvertedWorkspaceWavelength = "ConvertedWorkspaceWavelength";
    };
  };
  struct SumDetectors {
    static constexpr const char *name = "TaskSumDetectors";
    struct Outputs {
      static constexpr const char *SummedWorkspace = "SummedWorkspace";
    };
  };
  struct BackgroundSubtraction {
    static constexpr const char *name = "TaskBackgroundSubtraction";
    struct Outputs {
      static constexpr const char *BackgroundSubtractedWorkspace = "BackgroundSubtractedWorkspace";
    };
  };
  struct ExtractROI {
    static constexpr const char *name = "TaskExtractROI";
    struct Outputs {
      static constexpr const char *ExtractedROIWorkspace = "ExtractedROIWorkspace";
    };
  };
};

/** ReflectometryReductionOne3 : Reflectometry reduction of a single input TOF
 workspace to an IvsQ workspace. Version 3 of the algorithm.
 */
class MANTID_REFLECTOMETRY_DLL ReflectometryReductionOne3 : public ReflectometryWorkflowBase2,
                                                            public TaskBasedAlgorithm<ReflectometryReductionOne3> {
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

  /** Task based Algorithm declarations */
  class TaskBackgroundSubtraction final : public AlgorithmTask {
  public:
    explicit TaskBackgroundSubtraction(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, Tasks::BackgroundSubtraction::name) {
      setExpectedOutputs({Tasks::BackgroundSubtraction::Outputs::BackgroundSubtractedWorkspace});
      setDependantTask(Tasks::ExtractROI::name, Tasks::ExtractROI::Outputs::ExtractedROIWorkspace, "InputWorkspace");
    }
    void executeImpl() override;
  };

  class TaskConvertToWavelength final : public AlgorithmTask {
  public:
    explicit TaskConvertToWavelength(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, Tasks::ConvertToWavelength::name) {
      setExpectedOutputs({Tasks::ConvertToWavelength::Outputs::ConvertedWorkspaceWavelength});
      setDependantTask(Tasks::BackgroundSubtraction::name,
                       Tasks::BackgroundSubtraction::Outputs::BackgroundSubtractedWorkspace, "InputWorkspace");
      const auto taskSet = addDependantTaskSet();
      setDependantTask(Tasks::SumDetectors::name, Tasks::SumDetectors::Outputs::SummedWorkspace, "InputWorkspace",
                       taskSet);
    }
    void executeImpl() override;
  };

  class TaskNormalizeByMonitor final : public AlgorithmTask {
  public:
    explicit TaskNormalizeByMonitor(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, Tasks::NormalizeByMonitor::name) {
      setExpectedOutputs({Tasks::NormalizeByMonitor::Outputs::MonitorCorrectedWorkspace});
      setDependantTask(Tasks::SumDetectors::name, Tasks::SumDetectors::Outputs::SummedWorkspace, "InputWorkspace");
      const auto taskSet = addDependantTaskSet();
      setDependantTask(Tasks::ConvertToWavelength::name,
                       Tasks::ConvertToWavelength::Outputs::ConvertedWorkspaceWavelength, "InputWorkspace", taskSet);
    }
    void executeImpl() override;
  };

  class TaskNormalizeByTransmission final : public AlgorithmTask {
  public:
    explicit TaskNormalizeByTransmission(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, Tasks::NormalizeByTransmission::name) {
      setExpectedOutputs({Tasks::NormalizeByTransmission::Outputs::TransmissionCorrectedWorkspace});
      setDependantTask(Tasks::NormalizeByMonitor::name, Tasks::NormalizeByMonitor::Outputs::MonitorCorrectedWorkspace,
                       "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask(Tasks::ConvertToWavelength::name,
                       Tasks::ConvertToWavelength::Outputs::ConvertedWorkspaceWavelength, "InputWorkspace", taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask(Tasks::CropWavelength::name, Tasks::CropWavelength::Outputs::CroppedWorkspace, "InputWorkspace",
                       taskSet2);
    }
    void executeImpl() override;
  };

  class TaskNormalizeByAlgorithm final : public AlgorithmTask {
  public:
    explicit TaskNormalizeByAlgorithm(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, Tasks::NormalizeByAlgorithm::name) {
      setExpectedOutputs({Tasks::NormalizeByAlgorithm::Outputs::AlgorithmCorrectedWorkspace});
      setDependantTask(Tasks::NormalizeByMonitor::name, Tasks::NormalizeByMonitor::Outputs::MonitorCorrectedWorkspace,
                       "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask(Tasks::ConvertToWavelength::name,
                       Tasks::ConvertToWavelength::Outputs::ConvertedWorkspaceWavelength, "InputWorkspace", taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask(Tasks::CropWavelength::name, Tasks::CropWavelength::Outputs::CroppedWorkspace, "InputWorkspace",
                       taskSet2);
    }
    void executeImpl() override;
  };

  class TaskExtractROI final : public AlgorithmTask {
  public:
    explicit TaskExtractROI(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, Tasks::ExtractROI::name) {
      setExpectedOutputs({Tasks::ExtractROI::Outputs::ExtractedROIWorkspace});
    }
    void executeImpl() override;
  };

  class TaskSumDetectors final : public AlgorithmTask {
  public:
    explicit TaskSumDetectors(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, Tasks::SumDetectors::name) {
      setExpectedOutputs({Tasks::SumDetectors::Outputs::SummedWorkspace});
      setDependantTask(Tasks::ConvertToWavelength::name,
                       Tasks::ConvertToWavelength::Outputs::ConvertedWorkspaceWavelength, "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask(Tasks::BackgroundSubtraction::name,
                       Tasks::BackgroundSubtraction::Outputs::BackgroundSubtractedWorkspace, "InputWorkspace",
                       taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask(Tasks::ExtractROI::name, Tasks::ExtractROI::Outputs::ExtractedROIWorkspace, "InputWorkspace",
                       taskSet2);
    }
    void executeImpl() override;
  };

  class TaskSumDetectorsInQ final : public AlgorithmTask {
  public:
    explicit TaskSumDetectorsInQ(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, Tasks::SumDetectorsInQ::name) {
      setExpectedOutputs({Tasks::SumDetectorsInQ::Outputs::QSummedWorkspace});
      setDependantTask(Tasks::NormalizeByMonitor::name, Tasks::NormalizeByMonitor::Outputs::MonitorCorrectedWorkspace,
                       "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask(Tasks::NormalizeByTransmission::name,
                       Tasks::NormalizeByTransmission::Outputs::TransmissionCorrectedWorkspace, "InputWorkspace",
                       taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask(Tasks::ConvertToWavelength::name,
                       Tasks::ConvertToWavelength::Outputs::ConvertedWorkspaceWavelength, "InputWorkspace", taskSet2);
    }
    void executeImpl() override;
  };

  class TaskCropWavelength final : public AlgorithmTask {
  public:
    explicit TaskCropWavelength(ReflectometryReductionOne3 *parent)
        : AlgorithmTask(parent, Tasks::CropWavelength::name) {
      setExpectedOutputs({Tasks::CropWavelength::Outputs::CroppedWorkspace});
      setDependantTask(Tasks::NormalizeByMonitor::name, Tasks::NormalizeByMonitor::Outputs::MonitorCorrectedWorkspace,
                       "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask(Tasks::NormalizeByTransmission::name,
                       Tasks::NormalizeByTransmission::Outputs::TransmissionCorrectedWorkspace, "InputWorkspace",
                       taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask(Tasks::SumDetectorsInQ::name, Tasks::SumDetectorsInQ::Outputs::QSummedWorkspace,
                       "InputWorkspace", taskSet2);
      const auto taskSet3 = addDependantTaskSet();
      setDependantTask(Tasks::ConvertToWavelength::name,
                       Tasks::ConvertToWavelength::Outputs::ConvertedWorkspaceWavelength, "InputWorkspace", taskSet3);
    }
    void executeImpl() override;
  };

  class TaskConvertToQ final : public AlgorithmTask {
  public:
    explicit TaskConvertToQ(ReflectometryReductionOne3 *parent) : AlgorithmTask(parent, Tasks::ConvertToQ::name) {
      setExpectedOutputs({Tasks::ConvertToQ::Outputs::ConvertedWorkspaceQ});
      setDependantTask(Tasks::CropWavelength::name, Tasks::CropWavelength::Outputs::CroppedWorkspace, "InputWorkspace");
      const auto taskSet1 = addDependantTaskSet();
      setDependantTask(Tasks::NormalizeByTransmission::name,
                       Tasks::NormalizeByTransmission::Outputs::TransmissionCorrectedWorkspace, "InputWorkspace",
                       taskSet1);
      const auto taskSet2 = addDependantTaskSet();
      setDependantTask(Tasks::NormalizeByAlgorithm::name,
                       Tasks::NormalizeByAlgorithm::Outputs::AlgorithmCorrectedWorkspace, "InputWorkspace", taskSet2);
    }
    void executeImpl() override;
  };

  std::vector<std::string> constructTaskExecutionOrder() override;
};

} // namespace Reflectometry
} // namespace Mantid
