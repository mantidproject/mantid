// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
namespace Mantid {
namespace Algorithms {

/** ClearCache : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL ClearCache final : public API::Algorithm {
public:
  const std::string name() const override final;
  int version() const override final;
  const std::vector<std::string> seeAlso() const override { return {"CleanFileCache"}; }
  const std::string category() const override final;
  const std::string summary() const override final;

private:
  void init() override final;
  void exec() override final;
  int deleteFiles(const std::string &path, const std::string &pattern) const;
};

} // namespace Algorithms
} // namespace Mantid
