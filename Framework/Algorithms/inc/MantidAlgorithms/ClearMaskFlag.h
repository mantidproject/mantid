// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CLEARMASKFLAG_H_
#define MANTID_ALGORITHMS_CLEARMASKFLAG_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ClearMaskFlag : Delete the mask flag/bit on all spectra in a workspace
*/
class DLLExport ClearMaskFlag : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MaskDetectors"};
  }
  const std::string category() const override;
  /// Algorithm's summary
  const std::string summary() const override {
    return "Delete the mask flag/bit on all spectra in a workspace.";
  }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CLEARMASKFLAG_H_ */
