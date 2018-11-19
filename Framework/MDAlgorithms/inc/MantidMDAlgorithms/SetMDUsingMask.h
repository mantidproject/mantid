// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_SETMDUSINGMASK_H_
#define MANTID_MDALGORITHMS_SETMDUSINGMASK_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Algorithm to set a MDHistoWorkspace in points determined by a mask boolean
  MDHistoWorkspace.

  @date 2011-11-10
*/
class DLLExport SetMDUsingMask : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Algorithm to set a MDHistoWorkspace in points determined by a mask "
           "boolean MDHistoWorkspace.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SETMDUSINGMASK_H_ */
