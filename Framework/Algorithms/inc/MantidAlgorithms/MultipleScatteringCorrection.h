// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/MultipleScatteringCorrectionDistGraber.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Algorithms {

using namespace Geometry;

/** MultipleScatteringCorrection : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL MultipleScatteringCorrection : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MultipleScatteringCorrection"; };

  /// Algorithm's version
  int version() const override { return 1; };

  /// Algorithm's category
  const std::string category() const override { return "CorrectionFunctions"; };

  /// Algorithm's summary
  const std::string summary() const override {
    return "Calculate Multiple Scattering Correction using numerical integration with the assumption of"
           "elastic and isotropic scattering only.";
  };

  /// Algorithm's see also
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateCarpenterSampleCorrection", "CarpenterSampleCorrection", "MayersSampleCorrection"};
  };

protected:
  API::MatrixWorkspace_sptr m_inputWS; ///< A pointer to the input workspace
  Kernel::V3D m_beamDirection;         ///< The direction of the beam.
  int64_t m_num_lambda;                ///< The number of points in wavelength, the rest is interpolated linearly
  double m_sampleElementSize;          ///< The size of the integration element for sample in meters
  double m_containerElementSize;       ///< the size of the integration element for container in meters

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  void parseInputs();
  void calculateSingleComponent(const API::MatrixWorkspace_sptr &outws, const Geometry::IObject &shape,
                                const double elementSize);
  void calculateSampleAndContainer(const API::MatrixWorkspace_sptr &outws);
  // For single component case
  void calculateLS1s(const MultipleScatteringCorrectionDistGraber &distGraber, //
                     std::vector<double> &LS1s,                                //
                     const Geometry::IObject &shape) const;
  void calculateL12s(const MultipleScatteringCorrectionDistGraber &distGraber, //
                     std::vector<double> &L12s,                                //
                     const Geometry::IObject &shape);
  void calculateL2Ds(const MultipleScatteringCorrectionDistGraber &distGraber, //
                     const IDetector &detector,                                //
                     std::vector<double> &L2Ds,                                //
                     const Geometry::IObject &shape) const;
  void pairWiseSum(double &A1, double &A2,                                   //
                   const double linearCoefAbs,                               //
                   const MultipleScatteringCorrectionDistGraber &distGraber, //
                   const std::vector<double> &LS1s,                          //
                   const std::vector<double> &L12s,                          //
                   const std::vector<double> &L2Ds,                          //
                   const int64_t startIndex, const int64_t endIndex) const;
  // For sample and container case
  void calculateLS1s(const MultipleScatteringCorrectionDistGraber &distGraberContainer, //
                     const MultipleScatteringCorrectionDistGraber &distGraberSample,    //
                     std::vector<double> &LS1sContainer,                                //
                     std::vector<double> &LS1sSample,                                   //
                     const Geometry::IObject &shapeContainer,                           //
                     const Geometry::IObject &shapeSample) const;
  void calculateL12s(const MultipleScatteringCorrectionDistGraber &distGraberContainer, //
                     const MultipleScatteringCorrectionDistGraber &distGraberSample,    //
                     std::vector<double> &L12sContainer,                                //
                     std::vector<double> &L12sSample,                                   //
                     const Geometry::IObject &shapeContainer,                           //
                     const Geometry::IObject &shapeSample);
  void calculateL2Ds(const MultipleScatteringCorrectionDistGraber &distGraberContainer,     //
                     const MultipleScatteringCorrectionDistGraber &distGraberSample,        //
                     const IDetector &detector,                                             //
                     std::vector<double> &container_L2Ds, std::vector<double> &sample_L2Ds, //
                     const Geometry::IObject &shapeContainer, const Geometry::IObject &shapeSample) const;

  void pairWiseSum(double &A1, double &A2,                                                         //
                   const double linearCoefAbsContainer, const double linearCoefAbsSample,          //
                   const int64_t numVolumeElementsContainer, const int64_t numVolumeElementsTotal, //
                   const double totScatterCoefContainer, const double totScatterCoefSample,        //
                   const std::vector<double> &elementVolumes, const std::vector<double> &LS1sContainer,
                   const std::vector<double> &LS1sSample, const std::vector<double> &L12sContainer,
                   const std::vector<double> &L12sSample, const std::vector<double> &L2DsContainer,
                   const std::vector<double> &L2DsSample, const int64_t startIndex, const int64_t endIndex) const;

  int64_t m_xStep; ///< The step in bin number between adjacent points for linear interpolation
};

} // namespace Algorithms
} // namespace Mantid
