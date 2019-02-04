// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATEFLATEVENTWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEFLATEVENTWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** CreateFlatEventWorkspace : Creates a flat event workspace that can be used
  for background removal.
*/
class DLLExport CreateFlatEventWorkspace : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates a flat event workspace that can be used for background "
           "removal.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATEFLATEVENTWORKSPACE_H_ */
