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
DataItem::DataItem() { m_lock = std::make_unique<Poco::RWLock>(); }

/** Copy constructor
 */
DataItem::DataItem(const DataItem & /*other*/) {
  // Always make a unique lock!
  m_lock = std::make_unique<Poco::RWLock>();
}

/**
 * Destructor. Required in cpp do avoid linker errors when other projects try to
 * inherit from DataItem
 */
DataItem::~DataItem() {}

/** Private method to access the RWLock object.
 *
 * @return the RWLock object.
 */
Poco::RWLock *DataItem::getLock() const { return m_lock.get(); }

} // namespace Kernel
} // namespace Mantid
