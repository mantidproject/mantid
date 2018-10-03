// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_INTEGRATEPEAKSHYBRID_H_
#define MANTID_CRYSTAL_INTEGRATEPEAKSHYBRID_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** IntegratePeaksHybrid : Integrate single crystal peaks algorithm
*/
class DLLExport IntegratePeaksHybrid : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"IntegratePeaksUsingClusters", "IntegratePeaksMDHKL",
            "IntegratePeaksMD", "IntegratePeaksCWSD"};
  }
  const std::string category() const override;

private:
  const std::string summary() const override;
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_INTEGRATEPEAKSHYBRID_H_ */