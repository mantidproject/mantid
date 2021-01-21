// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/** IntegratePeaksHybrid : Integrate single crystal peaks algorithm
 */
class MANTID_CRYSTAL_DLL IntegratePeaksHybrid : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"IntegratePeaksUsingClusters", "IntegratePeaksMDHKL", "IntegratePeaksMD", "IntegratePeaksCWSD"};
  }
  const std::string category() const override;

private:
  const std::string summary() const override;
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
