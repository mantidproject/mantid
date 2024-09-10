// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include <memory>
#include <utility>

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
**/

template <typename ChainableType> class DLLExport Chainable {
protected:
  /// Successor factory
  /// std::optional<std::unique_ptr<ChainableType>> m_successor;
  std::unique_ptr<ChainableType> m_successor;
  /// Provide option for derived classes to check successor and throw if bad
  virtual void checkSuccessor() const {}

public:
  /// Set the successor
  template <typename T> Chainable &setSuccessor(T &&successor) {
    m_successor = std::move(successor);
    checkSuccessor();
    return *m_successor;
  }
  bool hasSuccessor() const { return bool(m_successor); }
  virtual ~Chainable() = 0;
};

/// Keep our destructor pure virtual, but require an implementation.
template <typename ChainableType> Chainable<ChainableType>::~Chainable() = default;

} // namespace Kernel
} // namespace Mantid
