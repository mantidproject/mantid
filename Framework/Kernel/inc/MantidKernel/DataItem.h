#ifndef MANTID_KERNEL_DATAITEM_H_
#define MANTID_KERNEL_DATAITEM_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
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
}

namespace Mantid {
namespace Kernel {

/**
 This class forms the base class of any item that wishes to be stored in the
 analysis data service

 @author Martyn Gigg

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  virtual const std::string name() const = 0;
  /// Can this object be accessed from multiple threads safely
  virtual bool threadSafe() const = 0;
  /// Serializes the object to a string
  virtual const std::string toString() const = 0;
  //@}

protected:
  Poco::RWLock *getLock() const;

private:
  /// Multiple-reader/single-writer lock to restrict multithreaded
  /// access to the data item.
  Poco::RWLock *m_lock;

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
typedef boost::shared_ptr<DataItem> DataItem_sptr;
/// Shared pointer to a const DataItem
typedef boost::shared_ptr<const DataItem> DataItem_const_sptr;

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_DATAITEM_H_ */
