// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Extracts specified spectra from a workspace and places them in a new
  workspace. In contrast to ExtractSpectra version 1 this does not support
  cropping X at the same time.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_ALGORITHMS_DLL ExtractSpectra2 : public API::DistributedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  template <class T> void exec(const T &inputWS, const Indexing::SpectrumIndexSet &indexSet);
};

} // namespace Algorithms
} // namespace Mantid
