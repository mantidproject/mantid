// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_WEAKPTR_H_
#define MANTID_PYTHONINTERFACE_WEAKPTR_H_
/*

  This file declares the get_pointer template function to allow
  boost python to understand weak_pointers. It must be add to the
  boost namespace
*/
#include <boost/weak_ptr.hpp>
#include <stdexcept>

namespace boost {
/**
 * Boost.Python doesn't understand weak_ptrs out of the box. This acts an
 * intermediary
 * so that a bare pointer can be retrieved from the wrapper. The important
 * bit here is that the weak pointer won't allow the bare pointer to be
 * retrieved
 * unless the object it points to still exists
 * The name and arguments are dictated by boost
 * @param dataItem :: A reference to the weak_ptr
 * @return A bare pointer to the HeldType
 */
template <typename HeldType>
inline HeldType *get_pointer(const boost::weak_ptr<HeldType> &dataItem) {
  if (boost::shared_ptr<HeldType> lockedItem = dataItem.lock()) {
    return lockedItem
        .get(); // Safe as we can guarantee that another reference exists
  } else {
    throw std::runtime_error("Variable invalidated, data has been deleted.");
  }
}
} // namespace boost

#endif /* MANTID_PYTHONINTERFACE_WEAKPTR_H_ */
