// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToMDMinMaxGlobal : Algorithm to calculate limits for ConvertToMD
  transformation which can be observed using an instrument which covers whole
  MD-space
    The current version knows
*/
class DLLExport ConvertToMDMinMaxGlobal : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate limits for ConvertToMD transformation, achievable on a "
           "spheric instrument.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ConvertToMD"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
