// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAXENTENTROPY_H_
#define MANTID_ALGORITHMS_MAXENTENTROPY_H_

#include "MantidAlgorithms/DllConfig.h"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace Mantid {
namespace Algorithms {

/** MaxentEntropy : Abstract base class defining the necessary methods to
  calculate any type of entropy to be used by MaxEnt
*/
class MANTID_ALGORITHMS_DLL MaxentEntropy {
public:
  // Constructor
  MaxentEntropy() = default;
  // Destructor
  virtual ~MaxentEntropy() = default;
  // First derivative of the entropy
  virtual std::vector<double> derivative(const std::vector<double> &values,
                                         double background) = 0;
  // Second derivative of the entropy
  virtual std::vector<double>
  secondDerivative(const std::vector<double> &values, double background) = 0;
  // Corrects an invalid value
  virtual std::vector<double> correctValues(const std::vector<double> &value,
                                            double newValue) = 0;
};

using MaxentEntropy_sptr = boost::shared_ptr<MaxentEntropy>;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTENTROPY_H_ */
