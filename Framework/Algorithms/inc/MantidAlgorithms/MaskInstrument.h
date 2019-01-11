// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MASKINSTRUMENT_H_
#define MANTID_ALGORITHMS_MASKINSTRUMENT_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Mask specified detectors in an instrument. This is does NOT clear the data
  in associated spectra in the workspace.  To clear the data manually
  `ClearMaskedSpectra` can be called.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_ALGORITHMS_DLL MaskInstrument : public API::DistributedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MaskDetectors", "ClearMaskedSpectra"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MASKINSTRUMENT_H_ */
