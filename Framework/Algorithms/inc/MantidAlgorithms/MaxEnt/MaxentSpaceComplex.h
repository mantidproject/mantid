// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAXENTSPACECOMPLEX_H_
#define MANTID_ALGORITHMS_MAXENTSPACECOMPLEX_H_

#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"

namespace Mantid {
namespace Algorithms {

/** MaxentSpaceComplex : Defines a space of complex numbers.
 */
class MANTID_ALGORITHMS_DLL MaxentSpaceComplex : public MaxentSpace {
public:
  // Converts a given vector to a complex vector
  std::vector<double> toComplex(const std::vector<double> &values) override;
  // Converts to a complex vector
  std::vector<double> fromComplex(const std::vector<double> &values) override;
};

using MaxentSpaceComplex_sptr =
    boost::shared_ptr<Mantid::Algorithms::MaxentSpaceComplex>;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTSPACECOMPLEX_H_ */
