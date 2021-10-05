// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#ifndef Q_MOC_RUN
#include <memory>
#endif
#include <string>

// Poco forward declarations
namespace Poco {
class RWLock;
}

// Forward declaration to allow Friend class.
namespace Mantid {
namespace API {
class Algorithm;
}
} // namespace Mantid

namespace Mantid {
namespace Kernel {

/**
 This class forms the base class of any item that wishes to be stored in the
 analysis data service

 @author Martyn Gigg
 */
class MANTID_KERNEL_DLL DataItem {
public:
  DataItem();
  DataItem(const DataItem &other);
  virtual ~DataItem();

  /** @name Interface */
  //@{
  /// A string ID for the class
  virtual const std::string id() const = 0;
  /// The name of the object
  virtual const std::string &getName() const = 0;
  /// Can this object be accessed from multiple threads safely
  virtual bool threadSafe() const = 0;
  /// Serializes the object to a string
  virtual const std::string toString() const = 0;
  //@}

  /// Acquires a read lock. If another thread currently holds a write lock,
  /// waits until the write lock is released.
  void readLock();
  void unlock();

protected:
  Poco::RWLock *getLock() const;

private:
  /// Multiple-reader/single-writer lock to restrict multithreaded
  /// access to the data item.
  std::unique_ptr<Poco::RWLock> m_lock;

  /// Allow the ReadLock class direct access to the m_lock object.
  friend class ReadLock;
  /// Allow the WriteLock class direct access to the m_lock object.
  friend class WriteLock;
  /// Allow the Algorithm class but NOT its derived classes to get the lock
  /// object.
  friend class Mantid::API::Algorithm;
  /* WARNING: Do not add other friend classes unless you really know what
   * you are doing. Write locks are INTENTIONALLY restricted to
   * only the Algorithm base class.
   */
};

/// Shared pointer to a DataItem
using DataItem_sptr = std::shared_ptr<DataItem>;
/// Shared pointer to a const DataItem
using DataItem_const_sptr = std::shared_ptr<const DataItem>;

} // namespace Kernel
} // namespace Mantid
