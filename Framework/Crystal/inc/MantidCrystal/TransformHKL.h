// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_TRANSFORM_HKL_H_
#define MANTID_CRYSTAL_TRANSFORM_HKL_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {
/** TransformHKL : Algorithm to adjust the UB saved in the sample associated
    with the specified PeaksWorkspace, so the HKL values are reordered or
    otherwise transformed.

    @author Dennis Mikkelson
    @date   2012-04-24
  */
class DLLExport TransformHKL : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override;

  /// Algorithm's version for identification
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SortHKL"};
  }

  /// Algorithm's category for identification
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Specify a 3x3 matrix to apply to (HKL) vectors as a list of 9 "
           "comma separated numbers. Both the UB and HKL values will be "
           "updated";
  }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_TRANSFORM_HKL */
