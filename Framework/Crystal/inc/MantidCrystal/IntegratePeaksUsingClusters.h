// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERS_H_
#define MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** IntegratePeaksUsingClusters : Uses clustering to integrate peaks.
 */
class DLLExport IntegratePeaksUsingClusters : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate single crystal peaks using connected component analysis";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"IntegratePeaksHybrid", "IntegratePeaksMDHKL", "IntegratePeaksMD",
            "IntegratePeaksCWSD"};
  }
  const std::string category() const override;

private:
  Mantid::API::MDNormalization getNormalization();
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERS_H_ */
