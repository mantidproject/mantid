// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef USER_ALGORITHMS_PROPERTYALGORITHM_H_
#define USER_ALGORITHMS_PROPERTYALGORITHM_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** An example algorithm showing the use of properties.

    @author Roman Tolchenov, ISIS, RAL
    @date 01/05/2008
 */
class PropertyAlgorithm : public API::Algorithm {
public:
  /// no arg constructor
  PropertyAlgorithm() : API::Algorithm() {}
  /// virtual destructor
  ~PropertyAlgorithm() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PropertyAlgorithm"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Examples"; }
  // Algorithm's summary. Overriding a virtual method.
  const std::string summary() const override { return "Example summary text."; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*USER_ALGORITHMS_PROPERTYALGORITHM_H_*/
