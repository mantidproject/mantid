// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_ANNULARRINGABSORPTION_H_
#define MANTID_ALGORITHMS_ANNULARRINGABSORPTION_H_

#include "MantidAPI/Algorithm.h"

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
class DLLExport AnnularRingAbsorption : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  void attachSample(API::MatrixWorkspace_sptr &workspace);
  void runCreateSampleShape(API::MatrixWorkspace_sptr &workspace);
  std::string createSampleShapeXML(const Kernel::V3D &upAxis) const;
  const std::string cylinderXML(const std::string &id,
                                const Kernel::V3D &bottomCentre,
                                const double radius, const Kernel::V3D &axis,
                                const double height) const;
  void runSetSampleMaterial(API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr
  runMonteCarloAbsorptionCorrection(const API::MatrixWorkspace_sptr &workspace);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ANNULARRINGABSORPTION_H_ */
