// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/ProgressBase.h"

namespace Mantid {
namespace API {
class Algorithm;

/**
 Helper class for reporting progress from algorithms.

 @author Roman Tolchenov, Tessella Support Services plc
 @date 06/02/2009
 */
class MANTID_API_DLL Progress final : public Mantid::Kernel::ProgressBase {
public:
  Progress();
  Progress(Algorithm *alg, double start, double end, int numSteps);
  Progress(Algorithm *alg, double start, double end, int64_t numSteps);
  Progress(Algorithm *alg, double start, double end, size_t numSteps);
  void doReport(const std::string &msg = "") override;
  bool hasCancellationBeenRequested() const override;

private:
  /// Owning algorithm
  Algorithm *const m_alg;
};

} // namespace API
} // namespace Mantid
