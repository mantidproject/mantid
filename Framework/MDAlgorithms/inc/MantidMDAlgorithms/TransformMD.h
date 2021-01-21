// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Scale and/or offset the coordinates of a MDWorkspace

  @date 2012-01-18
*/
class DLLExport TransformMD : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Scale and/or offset the coordinates of a MDWorkspace"; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"InvertMDDim"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void reverse(signal_t *array, size_t arrayLength);
  Mantid::DataObjects::MDHistoWorkspace_sptr transposeMD(Mantid::DataObjects::MDHistoWorkspace_sptr &toTranspose,
                                                         const std::vector<int> &axes);
  template <typename MDE, size_t nd> void doTransform(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  std::vector<double> m_scaling;
  std::vector<double> m_offset;
};

} // namespace MDAlgorithms
} // namespace Mantid
