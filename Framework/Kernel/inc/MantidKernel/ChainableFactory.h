// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Chainable.h"
#include "MantidKernel/DllConfig.h"
#include <memory>
#include <stdexcept>

namespace Mantid {
namespace Kernel {

/** ChainableFactory : Chain of Responsiblity generic factory
 */
template <typename Factory, typename Product, typename Argument>
class MANTID_KERNEL_DLL ChainableFactory : public Chainable<Factory> {
public:
  /**
   * Factory method wrapper. Wraps results in smart pointer.
   * @param argument : creational arguments
   * @return Product
   */
  std::unique_ptr<Product> create(const Argument &argument) const {
    if (this->canInterpret(argument)) {
      return std::unique_ptr<Product>(this->createRaw(argument));
    } else {
      if (this->hasSuccessor()) {
        return Chainable<Factory>::m_successor->create(argument);
      } else {
        throw std::invalid_argument("No successor MDUnitFactory");
      }
    }
  }

private:
  /// Create the product
  virtual Product *createRaw(const Argument &argument) const = 0;

  /// Indicate an ability to intepret the string
  virtual bool canInterpret(const Argument &unitString) const = 0;
};

} // namespace Kernel
} // namespace Mantid
