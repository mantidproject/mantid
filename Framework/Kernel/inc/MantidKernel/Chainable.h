#ifndef MANTID_KERNEL_CHAINABLE_H_
#define MANTID_KERNEL_CHAINABLE_H_

#include "MantidKernel/System.h"
#include <memory>

namespace Mantid {
namespace Kernel {

/** Chainable

 CRTP class

 Chainable gives the ability to set successors, but chainable items do not
define create. This is important
 because the return from setSuccessor should not be the factory directly.
Otherwise you could do this

 factory.setSuccessor(new Factory).create()

 and create would be bypass the chain of resposibility, which should be executed
along the chain top to bottom.

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
**/

template <typename ChainableType> class DLLExport Chainable {
protected:
  /// Successor factory
  /// boost::optional<std::unique_ptr<ChainableType>> m_successor;
  std::unique_ptr<ChainableType> m_successor;
  /// Provide option for derived classes to check successor and throw if bad
  virtual void checkSuccessor() const {};

public:
  /// Set the successor
  Chainable &setSuccessor(std::unique_ptr<ChainableType> &successor) {
    m_successor = std::move(successor);
    checkSuccessor();
    return *m_successor;
  }
  Chainable &setSuccessor(std::unique_ptr<ChainableType> &&successor) {
    m_successor = std::move(successor);
    checkSuccessor();
    return *m_successor;
  }
  bool hasSuccessor() const { return m_successor.get() != NULL; }
  virtual ~Chainable() = 0;
};

/// Keep our destructor pure virtual, but require an implementation.
template <typename ChainableType> Chainable<ChainableType>::~Chainable() {}

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CHAINABLE_H_ */
