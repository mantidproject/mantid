// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MultiThreaded.h"

#ifndef Q_MOC_RUN
#include <memory>
#endif

#include <mutex>
#include <vector>

namespace Mantid {
namespace Kernel {
/**
  \class cow_ptr
  \brief Implements a copy on write data template
  \version 1.0
  \date February 2006
  \author S.Ansell

  This version works only on data that is created via new().
  It is thread safe and works in the Standard template
  libraries (but appropriate functionals are needed for
  sorting etc.).

  Renamed from RefControl on the 11/12/2007,
  as it was agreed that copy on write pointer better
  described the functionality of this class.

  The underlying data can be accessed via the normal pointer
  semantics but call the access function if the data is required
  to be modified.

*/
template <typename DataType> class cow_ptr {
public:
  using ptr_type = std::shared_ptr<DataType>; ///< typedef for the storage
  using value_type = DataType;                ///< typedef for the data type

private:
  ptr_type Data; ///< Real object Ptr
  std::mutex copyMutex;

public:
  cow_ptr(ptr_type &&resourceSptr) noexcept;
  cow_ptr(const ptr_type &resourceSptr) noexcept;
  explicit cow_ptr(DataType *resourcePtr);
  cow_ptr();
  /// Constructs a cow_ptr with no managed object, i.e. empty cow_ptr.
  constexpr cow_ptr(std::nullptr_t) noexcept : Data(nullptr) {}
  cow_ptr(const cow_ptr<DataType> &) noexcept;
  // Move is hand-written, since std::mutex member prevents auto-generation.
  cow_ptr(cow_ptr<DataType> &&other) noexcept : Data(std::move(other.Data)) {}
  cow_ptr<DataType> &operator=(const cow_ptr<DataType> &) noexcept;
  // Move is hand-written, since std::mutex member prevents auto-generation.
  cow_ptr<DataType> &operator=(cow_ptr<DataType> &&rhs) noexcept {
    Data = std::move(rhs.Data);
    return *this;
  }
  cow_ptr<DataType> &operator=(const ptr_type &) noexcept;

  /// Returns the stored pointer.
  const DataType *get() const noexcept { return Data.get(); }

  /// Checks if *this stores a non-null pointer, i.e. whether get() != nullptr.
  explicit operator bool() const noexcept { return bool(Data); }

  /// Returns the number of different shared_ptr instances (this included)
  /// managing the current object. If there is no managed object, 0 is returned.
  long use_count() const noexcept { return Data.use_count(); }

  /// Checks if *this is the only shared_ptr instance managing the current
  /// object
  bool unique() const noexcept { return Data.use_count() == 1; }

  const DataType &operator*() const { return *Data; }       ///< Pointer dereference access
  const DataType *operator->() const { return Data.get(); } ///< indirectrion dereference access
  bool operator==(const cow_ptr<DataType> &A) const noexcept { return Data == A.Data; } ///< Based on ptr equality
  bool operator!=(const cow_ptr<DataType> &A) const noexcept { return Data != A.Data; } ///< Based on ptr inequality
  DataType &access();
};

/**
 Constructor : creates a new cow_ptr around the resource
 resource is a sink.
 */
template <typename DataType> cow_ptr<DataType>::cow_ptr(DataType *resourcePtr) : Data(resourcePtr) {}

/**
  Constructor : creates new data() object
*/
template <typename DataType> cow_ptr<DataType>::cow_ptr() : Data(std::make_shared<DataType>()) {}

/**
  Copy constructor : double references the data object
  @param A :: object to copy
*/
// Note: Need custom implementation, since std::mutex is not copyable.
template <typename DataType>
cow_ptr<DataType>::cow_ptr(const cow_ptr<DataType> &A) noexcept : Data(std::atomic_load(&A.Data)) {}

/**
  Assignment operator : double references the data object
  maybe drops the old reference.
  @param A :: object to copy
  @return *this
*/
// Note: Need custom implementation, since std::mutex is not copyable.
template <typename DataType> cow_ptr<DataType> &cow_ptr<DataType>::operator=(const cow_ptr<DataType> &A) noexcept {
  if (this != &A) {
    std::atomic_store(&Data, std::atomic_load(&A.Data));
  }
  return *this;
}

/**
  Assignment operator : double references the data object
  maybe drops the old reference.
  @param A :: object to copy
  @return *this
*/
template <typename DataType> cow_ptr<DataType> &cow_ptr<DataType>::operator=(const ptr_type &A) noexcept {
  if (this->Data != A) {
    std::atomic_store(&Data, std::atomic_load(&A));
  }
  return *this;
}

/**
  Access function.
  If data is shared, creates a copy of Data so that it can be modified.

  In certain situations this function is not thread safe. Specifically it is not
  thread
  safe in the presence of a simultaneous cow_ptr copy. Copies of the underlying
  data are only
  made when the reference count > 1.

  @return new copy of *this, if required
*/
template <typename DataType> DataType &cow_ptr<DataType>::access() {
  // Use a double-check for sharing so that we only acquire the lock if
  // absolutely necessary
  if (Data.use_count() > 1) {
    std::lock_guard<std::mutex> lock{copyMutex};
    // Check again because another thread may have taken copy and dropped
    // reference count since previous check
    if (Data.use_count() > 1) {
      std::atomic_store(&Data, std::make_shared<DataType>(*Data));
    }
  }
  return *Data;
}

template <typename DataType> cow_ptr<DataType>::cow_ptr(ptr_type &&resourceSptr) noexcept {
  std::atomic_store(&this->Data, std::move(resourceSptr));
}

template <typename DataType> cow_ptr<DataType>::cow_ptr(const ptr_type &resourceSptr) noexcept {
  std::atomic_store(&this->Data, std::atomic_load(&resourceSptr));
}

} // NAMESPACE Kernel

/// typedef for the data storage used in Mantid matrix workspaces
using MantidVec = std::vector<double>;

/// typedef for the pointer to data storage used in Mantid matrix workspaces
using MantidVecPtr = Kernel::cow_ptr<MantidVec>;

} // NAMESPACE Mantid
