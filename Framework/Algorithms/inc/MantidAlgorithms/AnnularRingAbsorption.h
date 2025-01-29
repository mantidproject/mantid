// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
namespace Mantid {
//-----------------------------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------------------------
namespace Kernel {
class Material;
class V3D;
} // namespace Kernel

namespace Algorithms {

/**
  Constructs a hollow sample shape, defines material for the sample and runs the
  MonteCarloAbsorption algorithm.
*/
class MANTID_ALGORITHMS_DLL AnnularRingAbsorption final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"AbsorptionCorrection"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  void attachSample(const API::MatrixWorkspace_sptr &workspace);
  void runCreateSampleShape(const API::MatrixWorkspace_sptr &workspace);
  std::string createSampleShapeXML(const Kernel::V3D &upAxis) const;
  const std::string cylinderXML(const std::string &id, const Kernel::V3D &bottomCentre, const double radius,
                                const Kernel::V3D &axis, const double height) const;
  void runSetSampleMaterial(const API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr runMonteCarloAbsorptionCorrection(const API::MatrixWorkspace_sptr &workspace);
};

} // namespace Algorithms
} // namespace Mantid
