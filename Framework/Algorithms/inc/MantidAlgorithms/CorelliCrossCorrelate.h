// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CORELLICROSSCORRELATE_H_
#define MANTID_ALGORITHMS_CORELLICROSSCORRELATE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** CorelliCrossCorrelate : TODO: DESCRIPTION
*/
class DLLExport CorelliCrossCorrelate : public API::Algorithm {
public:
  const std::string name() const override { return "CorelliCrossCorrelate"; };
  int version() const override { return 1; };
  const std::string category() const override {
    return "Diffraction\\Calibration;Events";
  };
  const std::string summary() const override {
    return "Cross-correlation calculation for the elastic signal from Corelli.";
  };

private:
  std::map<std::string, std::string> validateInputs() override;
  void init() override;
  void exec() override;

  /// Input workspace
  DataObjects::EventWorkspace_const_sptr inputWS;
  DataObjects::EventWorkspace_sptr outputWS;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CORELLICROSSCORRELATE_H_ */
