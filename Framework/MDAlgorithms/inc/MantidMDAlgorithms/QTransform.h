// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

/** QTransform : Base algorithm for transforming |Q| values in an MD workspace.
 */
class MANTID_MDALGORITHMS_DLL QTransform : public API::Algorithm {
public:
protected:
  std::map<std::string, std::string> validateInputs() override;
  void init() override;
  void exec() override;

  template <typename MDE, size_t nd>
  void applyCorrection(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr);

  // correction method to be implemented by derived classes. Input parameter is |Q|^2.
  virtual double correction(const double) const = 0;

private:
  size_t m_numQDims;
};

} // namespace MDAlgorithms
} // namespace Mantid
