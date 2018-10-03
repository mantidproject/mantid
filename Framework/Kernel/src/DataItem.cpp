// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DataItem.h"
#include <Poco/RWLock.h>

namespace Mantid {
namespace Kernel {

/** Default constructor
 */
DataItem::DataItem() { m_lock = new Poco::RWLock(); }

/** Copy constructor
 */
DataItem::DataItem(const DataItem & /*other*/) {
  // Always make a unique lock!
  m_lock = new Poco::RWLock();
}

/**
 * Destructor. Required in cpp do avoid linker errors when other projects try to
 * inherit from DataItem
 */
DataItem::~DataItem() {
  delete m_lock;
  m_lock = nullptr;
}

/** Private method to access the RWLock object.
 *
 * @return the RWLock object.
 */
Poco::RWLock *DataItem::getLock() const { return m_lock; }

} // namespace Kernel
} // namespace Mantid
