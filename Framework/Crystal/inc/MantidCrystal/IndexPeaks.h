// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/**
 * Implements an algorithm for indexing main and satellites peaks
 * in single crystal peaks.
 */
class MANTID_CRYSTAL_DLL IndexPeaks : public API::Algorithm {
public:
  const std::string name() const override { return "IndexPeaks"; }
  const std::string summary() const override {
    return "Index the peaks using the UB from the sample. Leave MaxOrder at "
           "default value 0 to get this value, and ModVectors (unless "
           "overridden) from the sample.";
  }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"IndexSXPeaks"}; }
  const std::string category() const override { return "Crystal\\Peaks"; }
  std::map<std::string, std::string> validateInputs() override;

protected:
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
