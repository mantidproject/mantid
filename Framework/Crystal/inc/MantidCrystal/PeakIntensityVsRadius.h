// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_PEAKINTENSITYVSRADIUS_H_
#define MANTID_CRYSTAL_PEAKINTENSITYVSRADIUS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** Calculate the integrated intensity of peaks vs integration radius.

  @date 2011-12-02
*/
class DLLExport PeakIntensityVsRadius : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the integrated intensity of peaks vs integration radius.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"PeakIntegration"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PEAKINTENSITYVSRADIUS_H_ */
