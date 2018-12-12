// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_INTEGRATEBYCOMPONENT_H_
#define MANTID_ALGORITHMS_INTEGRATEBYCOMPONENT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** IntegrateByComponent : The algorithm integrates up the instrument hierarchy,
  and each pixel will contain the average value for the component
*/
class DLLExport IntegrateByComponent : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Averages up the instrument hierarchy.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Integration"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// method to check which spectra should be averaged
  std::vector<std::vector<size_t>> makeMap(API::MatrixWorkspace_sptr countsWS,
                                           int parents);
  /// method to create the map with all spectra
  std::vector<std::vector<size_t>>
  makeInstrumentMap(API::MatrixWorkspace_sptr countsWS);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_INTEGRATEBYCOMPONENT_H_ */
