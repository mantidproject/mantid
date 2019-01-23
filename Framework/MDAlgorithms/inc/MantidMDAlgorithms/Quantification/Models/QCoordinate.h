// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_QCOORDINATE_H_
#define MANTID_MDALGORITHMS_QCOORDINATE_H_

#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

namespace Mantid {
namespace MDAlgorithms {

/**
 * Defines a diagnostic model that simply returns
 * the value of H,K,L,Energy as the weight depending on
 * the input parameter
 */
class DLLExport QCoordinate : public ForegroundModel {

public:
  /// Default constructor
  QCoordinate();

private:
  /// String name of the model
  std::string name() const override { return "QCoordinate"; }

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

  /// Which coordinate has been chosen
  size_t m_coord;
};
} // namespace MDAlgorithms
} // namespace Mantid
#endif /* MANTID_MDALGORITHMS_QCOORDINATE_H_ */
