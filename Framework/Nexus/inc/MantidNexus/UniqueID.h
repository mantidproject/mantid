#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"
#include <atomic>

// Forward declarations for needed HDF5 functions
// NOTE declare extern "C" to prevent conflict with actual declaration
// NOTE use MYH5DLL, set to match HDF5's H5_DLL, to allow Windows build
#if defined(_MSC_VER) // on Windows builds
#define MYH5DLL __declspec(dllimport)
#else
#define MYH5DLL // gcc and clang do not need a DLL specifier
#endif
extern "C" {
MYH5DLL herr_t H5Iis_valid(hid_t);
MYH5DLL herr_t H5Fclose(hid_t);
MYH5DLL herr_t H5garbage_collect();
}

namespace Mantid::Nexus {

/// @brief an ID that HDF5 will always consider invalid
constexpr hid_t INVALID_HID{-1};

/**
 * @class Hdf5ID
 * @brief A very simple wrapper that holds an HDF5 object through its hid_t.
 */
template <herr_t (*const D)(hid_t)> class Hdf5ID {
protected:
  hid_t m_id;

  void close();

public:
  // constructors
  Hdf5ID() noexcept : m_id(INVALID_HID) {}
  Hdf5ID(hid_t const id) noexcept : m_id(id) {}

  // comparators
  bool operator==(hid_t const id) const { return m_id == id; }
  bool operator!=(hid_t const id) const { return m_id != id; }
  bool operator<=(hid_t const id) const { return m_id <= id; }
  bool operator<(hid_t const id) const { return m_id < id; }

  /// @brief Return the managed HDF5 handle
  /// @return the managed HDF5 handle
  hid_t get() const { return m_id; }
  operator hid_t() const { return m_id; }
  /// @brief Return whether the UniqueId corresponds to a valid HDF5 object
  /// @return true if it is valid; otherwise false; on error, false
  bool isValid() const {
    // fail early condition
    if (m_id <= 0) {
      return false;
    } else {
      return H5Iis_valid(m_id) > 0;
    }
  }
};

/// @brief Close the held object ID
template <herr_t (*const closer)(hid_t)> inline void Hdf5ID<closer>::close() {
  if (this->isValid()) {
    closer(this->m_id);
    this->m_id = INVALID_HID;
  }
}

/// @brief Close a held file ID, and also call garbage collection
template <> inline void Hdf5ID<&H5Fclose>::close() {
  if (this->isValid()) {
    H5Fclose(this->m_id);
    this->m_id = INVALID_HID;
    // call garbage collection to close any and all open objects on this file
    H5garbage_collect();
  }
}

// ******************************************************************
// UNIQUE ID
// ******************************************************************

/**
 * @class UniqueID
 * @brief A wrapper class for managing HDF5 object handles (hid_t).
 * @details The UniqueID class is designed to manage the lifecycle of HDF5 object handles (hid_t),
 * ensuring that the handle is properly closed when the UniqueID object is destroyed.
 * This helps prevent resource leaks and ensures proper cleanup of HDF5 resources.
 */
template <herr_t (*const D)(hid_t)> class UniqueID : public Hdf5ID<D> {
private:
  // prohibit copying a unique ID
  UniqueID(UniqueID<D> const &) = delete;
  UniqueID &operator=(UniqueID<D> const &) = delete;

public:
  // constructors / destructor
  UniqueID() : Hdf5ID<D>() {}
  UniqueID(hid_t const id) : Hdf5ID<D>(id) {}
  UniqueID(UniqueID<D> &&uid) noexcept : Hdf5ID<D>(uid.m_id) { uid.m_id = INVALID_HID; }
  ~UniqueID() { this->close(); }

  // assignment
  UniqueID<D> &operator=(hid_t const);
  UniqueID<D> &operator=(UniqueID<D> &&);

  void reset() { reset(INVALID_HID); };
  void reset(hid_t const);
  void reset(UniqueID<D> const &) = delete;
  void reset(UniqueID<D> &&);
  hid_t release();
};

/// @brief  Release hold on the managed ID; it will not be closed by this UniqueID
/// @return the managed ID
template <herr_t (*const D)(hid_t)> inline hid_t UniqueID<D>::release() {
  hid_t tmp = this->m_id;
  this->m_id = INVALID_HID;
  return tmp;
}

/// @brief  Close the existing ID and replace with the new ID; or, set to invalid
/// @param id The new ID to be held; defaults to invalid
template <herr_t (*const D)(hid_t)> inline void UniqueID<D>::reset(hid_t const id) {
  if (this->m_id != id) {
    this->close();
    this->m_id = id;
  }
}

/// @brief  Close the existing ID and replace with the new ID; or, set to invalid
/// @param uid a UniqueID being moved and releasing control to this UniqueID
template <herr_t (*const D)(hid_t)> inline void UniqueID<D>::reset(UniqueID<D> &&uid) {
  if (this != &uid) {
    reset(uid.m_id);
    uid.m_id = INVALID_HID;
  }
}

/// @brief Assign a HDF5 object ID to be managed
/// @param id : the ID to be managed
template <herr_t (*const D)(hid_t)> inline UniqueID<D> &UniqueID<D>::operator=(hid_t const id) {
  reset(id);
  return *this;
}

/// @brief Pass the HDF5 object ID from an existing UniqueID to another UniqueID
/// @param uid : the UniqueID previously managing the ID; it will lose ownership of the ID.
template <herr_t (*const D)(hid_t)> inline UniqueID<D> &UniqueID<D>::operator=(UniqueID<D> &&uid) {
  reset(std::move(uid));
  return *this;
}

// ******************************************************************
// SHARED ID
// ******************************************************************

/**
 * @class SharedID
 * @brief A wrapper class for managing HDF5 object handles (hid_t) that can be shared.
 * @details The SharedID class is designed to manage the lifecycle of HDF5 object handles (hid_t),
 * with multiple ownership, ensuring the handle is properly closed when all leashes to it are dropped.
 * This helps prevent resource leaks and ensures proper cleanup of HDF5 resources.
 */
template <herr_t (*const D)(hid_t)> class SharedID : public Hdf5ID<D> {
private:
  std::atomic<std::size_t> *m_leash_counts;
  void increment_leash_counts();
  void decrement_leash_counts();

public:
  // constructors / destructor
  SharedID() : Hdf5ID<D>(), m_leash_counts(nullptr) {}
  SharedID(hid_t id) : Hdf5ID<D>(id), m_leash_counts(this->isValid() ? new std::atomic<std::size_t>(1) : nullptr) {}
  SharedID(SharedID<D> const &uid) : Hdf5ID<D>(uid.m_id), m_leash_counts(uid.m_leash_counts) {
    increment_leash_counts();
  }
  SharedID(SharedID<D> &&uid) : Hdf5ID<D>(uid.m_id), m_leash_counts(uid.m_leash_counts) {
    uid.m_id = INVALID_HID;
    uid.m_leash_counts = nullptr;
  }
  ~SharedID() { decrement_leash_counts(); }

  SharedID<D> &operator=(hid_t const);
  SharedID<D> &operator=(SharedID<D> const &);
  SharedID<D> &operator=(SharedID<D> &&);

  /// @brief ensure two SharedIDs are tracking the same object
  bool operator==(SharedID<D> const &uid) const {
    if (m_leash_counts == uid.m_leash_counts) {
      return this->m_id == uid.m_id;
    } else {
      return false;
    }
  }

  /// @brief Returns the number of SharedID objects holding the same ID
  std::size_t use_count() const { return (m_leash_counts ? (*m_leash_counts).load() : 0); }

  void reset() { reset(INVALID_HID); };
  void reset(hid_t const);
  void reset(SharedID<D> const &);
  void reset(SharedID<D> &&);
};

/// @brief  Decrement the existing ID and replace with the new ID; or, set to invalid
/// @param id The new ID to be held; defaults to invalid
template <herr_t (*const D)(hid_t)> inline void SharedID<D>::reset(hid_t const id) {
  if (this->m_id != id) {
    decrement_leash_counts();
    this->m_id = id;
    m_leash_counts = nullptr;
    increment_leash_counts();
  }
}

/// @brief  Decrement the existing ID and replace with the new ID; or, set to invalid
/// @param uid A SharedID whose ID will be shared
template <herr_t (*const D)(hid_t)> inline void SharedID<D>::reset(SharedID<D> const &uid) {
  if (&uid != this && uid.m_id != this->m_id) {
    decrement_leash_counts();
    this->m_id = uid.m_id;
    m_leash_counts = uid.m_leash_counts;
    increment_leash_counts();
  }
}

/// @brief  Decrement the existing ID and replace with the new ID; or, set to invalid
/// @param uid A SharedID whose ID will be moved to here
template <herr_t (*const D)(hid_t)> inline void SharedID<D>::reset(SharedID<D> &&uid) {
  if (&uid != this && uid.m_id != this->m_id) {
    decrement_leash_counts();
    this->m_id = uid.m_id;
    m_leash_counts = uid.m_leash_counts;
    // unset the moved id
    uid.m_id = INVALID_HID;
    uid.m_leash_counts = nullptr;
    // no increment -- number of leashes is same
  }
}

template <herr_t (*const D)(hid_t)> inline void SharedID<D>::increment_leash_counts() {
  if (this->isValid()) {
    if (!m_leash_counts) {
      m_leash_counts = new std::atomic<std::size_t>(1);
    } else {
      (*m_leash_counts)++;
    }
  }
}

template <herr_t (*const D)(hid_t)> inline void SharedID<D>::decrement_leash_counts() {
  if (m_leash_counts) {
    // atomic-safe access the counts
    std::size_t prev_counts = (*m_leash_counts).fetch_sub(1);
    if (prev_counts == 1) {
      this->close();
      delete m_leash_counts;
      m_leash_counts = nullptr;
    } else if (prev_counts == 0) {
      m_leash_counts->store(0);
      delete m_leash_counts;
      m_leash_counts = nullptr;
    }
  }
}

template <herr_t (*const D)(hid_t)> inline SharedID<D> &SharedID<D>::operator=(hid_t id) {
  reset(id);
  return *this;
}

template <herr_t (*const D)(hid_t)> inline SharedID<D> &SharedID<D>::operator=(SharedID<D> const &uid) {
  reset(uid);
  return *this;
}

template <herr_t (*const D)(hid_t)> inline SharedID<D> &SharedID<D>::operator=(SharedID<D> &&uid) {
  reset(std::move(uid));
  return *this;
}

} // namespace Mantid::Nexus
