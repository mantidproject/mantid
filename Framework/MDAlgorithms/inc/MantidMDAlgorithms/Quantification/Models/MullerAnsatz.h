// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_MULLERANSATZ_H_
#define MANTID_MDALGORITHMS_MULLERANSATZ_H_

#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

namespace Mantid {
namespace MDAlgorithms {

/**
 * Defines the Muller Ansatz model of Ewings et al.
 * This is model 300 in TobyFit.
 */
class DLLExport MullerAnsatz : public ForegroundModel {
public:
  /// possible scattering chain directions
  enum ChainDirection { Along_a, Along_b, Along_c };
  /// possible magnetic form factor directions
  enum MagneticFFDirection { NormalTo_a, NormalTo_b, NormalTo_c, Isotropic };

  /// Calculates the intensity for the model for the current parameters.
  double scatteringIntensity(const API::ExperimentInfo &exptSetup,
                             const std::vector<double> &point) const override;
  /// Called when an attribute is set
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &attr) override;
  /// Returns the type of model
  ModelType modelType() const override { return Broad; }
  /// String name of the model
  std::string name() const override { return "MullerAnsatz"; }
  MullerAnsatz();

private:
  /// Setup the model
  void init() override;
  // direction of the magnetic chain wrt the lattice vectors
  ChainDirection m_ChainDirection;
  // direction of the magnetic form factor wrt the lattice vectors.
  MagneticFFDirection m_FFDirection;
};
} // namespace MDAlgorithms
} // namespace Mantid
#endif /* MANTID_MDALGORITHMS_STRONTIUM122_H_ */
