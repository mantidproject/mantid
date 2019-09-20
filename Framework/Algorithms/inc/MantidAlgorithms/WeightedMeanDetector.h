// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_WEIGHTEDMEANDETECTOR_H_
#define MANTID_ALGORITHMS_WEIGHTEDMEANDETECTOR_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

class DLLExport WeightedMeanDetector : public API::Algorithm {
public:
  WeightedMeanDetector() : API::Algorithm() {}
  virtual ~WeightedMeanDetector() {}
  const std::string name() const override { return "WeightedMeanDetector"; };
  int version() const override { return 1; };
  const std::string category() const override { return "Transforms\\Grouping"; };
  const std::string summary() const override {
    return "This Algorithm creates a weighted mean from multiple detectors.";
  };
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  const std::map<int, double> read_alf_file(std::string dir,
                                            size_t spectra_num);
  const std::map<int, std::vector<double>> read_lin_file(std::string dir,
                                                   size_t spectra_num);
  const std::map<int, std::vector<double>> read_lim_file(std::string dir,
                                                   size_t spectra_num);
  const API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr input,
                                  std::string params);
};


} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_WEIGHTEDMEANDETECTOR_H_*/
