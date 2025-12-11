#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"

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

/**
 * @class UniqueID
 * @brief A wrapper class for managing HDF5 object handles (hid_t).
 * @details The UniqueID class is designed to manage the lifecycle of HDF5 object handles (hid_t),
 * ensuring that the handle is properly closed when the UniqueID object is destroyed.
 * This helps prevent resource leaks and ensures proper cleanup of HDF5 resources.
 */
template <herr_t (*const D)(hid_t)> class UniqueID {
protected:
  hid_t m_id;

private:
  UniqueID(UniqueID<D> const &uid) = delete;
  UniqueID &operator=(UniqueID<D> const &) = delete;
  void close();

public:
  // constructors / destructor
  UniqueID() : m_id(INVALID_ID) {}
  UniqueID(hid_t const id) : m_id(id) {}
  ~UniqueID() { close(); }
  UniqueID(UniqueID<D> &&uid) noexcept : m_id(uid.m_id) { uid.m_id = INVALID_ID; }

  // assignment
  UniqueID<D> &operator=(hid_t const id);
  UniqueID<D> &operator=(UniqueID<D> &&uid);

  // comparators
  bool operator==(int const id) const { return static_cast<int>(m_id) == id; }
  bool operator<=(int const id) const { return static_cast<int>(m_id) <= id; }
  bool operator<(int const id) const { return static_cast<int>(m_id) < id; }

  // using the id
  operator hid_t() const { return m_id; }

  /// @brief Return the managed HDF5 handle
  /// @return the managed HDF5 handle
  hid_t get() const { return m_id; }

  hid_t release();
  void reset(hid_t const id = INVALID_ID);
  bool isValid() const;

  /// @brief represents an invalid ID value
  static hid_t constexpr INVALID_ID{-1};
};

/// @brief Return whether the UniqueId corresponds to a valid HDF5 object
/// @return true if it is valid; otherwise false; on error, false
template <herr_t (*const D)(hid_t)> inline bool UniqueID<D>::isValid() const {
  // fail early condition
  if (m_id < 0) {
    return false;
  } else {
    return H5Iis_valid(m_id) > 0;
  }
}

/// @brief Close the held ID by calling its deleter function
/// @tparam deleter
template <herr_t (*const deleter)(hid_t)> inline void UniqueID<deleter>::close() {
  if (isValid()) {
    deleter(this->m_id);
    this->m_id = INVALID_ID;
  }
}

/// @brief Close a ID corresponding to a file; and call garbage collection
template <> inline void UniqueID<&H5Fclose>::close() {
  if (isValid()) {
    H5Fclose(this->m_id);
    this->m_id = INVALID_ID;
    // call garbage collection to close any and all open objects on this file
    H5garbage_collect();
  }
}

/// @brief  Release hold on the managed ID; it will not be closed by this UniqueID
/// @return the managed ID
template <herr_t (*const D)(hid_t)> inline hid_t UniqueID<D>::release() {
  hid_t tmp = m_id;
  m_id = INVALID_ID;
  return tmp;
}

/// @brief  Close the existing ID and replace with the new ID; or, set to invalid
/// @param id The new ID to be held; defaults to invalid
template <herr_t (*const D)(hid_t)> inline void UniqueID<D>::reset(hid_t const id) {
  if (m_id != id) {
    close();
    m_id = id;
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
  if (this != &uid) {
    reset(uid.m_id);
    uid.m_id = INVALID_ID;
  }
  return *this;
}

} // namespace Mantid::Nexus
