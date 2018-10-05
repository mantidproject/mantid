// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAXENTSPACEREAL_H_
#define MANTID_ALGORITHMS_MAXENTSPACEREAL_H_

#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"

namespace Mantid {
namespace Algorithms {

/** MaxentSpaceReal : Defines the space of real numbers.
 */
class MANTID_ALGORITHMS_DLL MaxentSpaceReal : public MaxentSpace {
public:
  // Converts a real vector to a complex vector
  std::vector<double> toComplex(const std::vector<double> &values) override;
  // Converts a complex vector to a real vector
  std::vector<double> fromComplex(const std::vector<double> &values) override;
};

using MaxentSpaceReal_sptr =
    boost::shared_ptr<Mantid::Algorithms::MaxentSpaceReal>;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTSPACEREAL_H_ */
