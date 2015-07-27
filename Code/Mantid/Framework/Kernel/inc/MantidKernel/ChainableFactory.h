#ifndef MANTID_KERNEL_CHAINABLEFACTORY_H_
#define MANTID_KERNEL_CHAINABLEFACTORY_H_

#include "MantidKernel/Chainable.h"
#include "MantidKernel/System.h"
#include <memory>

namespace Mantid {
namespace Kernel {

/** ChainableFactory : Chain of Responsiblity generic factory

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <typename Factory, typename Product>
class DLLExport ChainableFactory : public Chainable<Factory> {
public:
  /// Destructor
  virtual ~ChainableFactory() {}

  /**
   * Factory method wrapper. Wraps results in smart pointer.
   * @param unitString : unit string to intepret
   * @return Product
   */
  std::unique_ptr<Product> create(const std::string &arguments) const {
    if (this->canInterpret(arguments)) {
      return std::unique_ptr<Product>(this->createRaw(arguments));
    } else {
      if (this->hasSuccessor()) {
        return (*Chainable<Factory>::m_successor)->create(arguments);
      } else {
        throw std::invalid_argument("No successor MDUnitFactory");
      }
    }
  }

private:
  /// Create the product
  virtual Product *createRaw(const std::string &argument) const = 0;

  /// Indicate an ability to intepret the string
  virtual bool canInterpret(const std::string &unitString) const = 0;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CHAINABLEFACTORY_H_ */
