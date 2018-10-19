// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_STRONTIUM122_H_
#define MANTID_MDALGORITHMS_STRONTIUM122_H_

#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

namespace Mantid {
namespace MDAlgorithms {

/**
 * Defines the Strontium-122 model of Ewings et al.
 * This is model 207 in TobyFit.
 */
class DLLExport Strontium122 : public ForegroundModel {
private:
  /// String name of the model
  std::string name() const override { return "Strontium122"; }

  /// Setup the model
  void init() override;
  /// Called when an attribute is set
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &attr) override;

  /// Returns the type of model
  ModelType modelType() const override { return Broad; }
  /// Calculates the intensity for the model for the current parameters.
  double scatteringIntensity(const API::ExperimentInfo &exptSetup,
                             const std::vector<double> &point) const override;

  /// Twin type attribute
  int m_twinType;
  /// MultEps attribute
  bool m_multEps;

public:
  Strontium122();
};
} // namespace MDAlgorithms
} // namespace Mantid
#endif /* MANTID_MDALGORITHMS_STRONTIUM122_H_ */
