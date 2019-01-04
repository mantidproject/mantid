// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEDYNAMICRANGE_H_
#define MANTID_ALGORITHMS_CALCULATEDYNAMICRANGE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** CalculateDynamicRange
 * Calculates the Qmin and Qmax of SANS workspace, sets to sample logs.
 */
class MANTID_ALGORITHMS_DLL CalculateDynamicRange : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void calculateQMinMax(API::MatrixWorkspace_sptr, const std::vector<size_t> &,
                        const std::string &);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALCULATEDYNAMICRANGE_H_ */
