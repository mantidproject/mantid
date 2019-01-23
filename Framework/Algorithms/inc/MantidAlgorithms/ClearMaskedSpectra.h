// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CLEARMASKEDSPECTRA_H_
#define MANTID_ALGORITHMS_CLEARMASKEDSPECTRA_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Clear counts (or events, if applicable) on all spectra that are fully
  masked.  A spectrum is fully masked if all of its associated detectors are
  masked, e.g., from a call to `MaskInstrument`.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_ALGORITHMS_DLL ClearMaskedSpectra
    : public API::DistributedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MaskDetectors", "MaskInstrument"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CLEARMASKEDSPECTRA_H_ */
