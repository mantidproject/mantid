#pragma once

#include "MantidNexus/NexusFile_fwd.h"

// external forward declare
#if defined(_MSC_VER) // on Windows builds
#define MYH5DLL __declspec(dllimport)
#else
#define MYH5DLL
#endif
extern "C" MYH5DLL herr_t H5Iis_valid(hid_t);

namespace Mantid::Nexus {

/**
 * \class UniqueID
 * \brief A wrapper class for managing HDF5 object handles (hid_t).
 *
 * The UniqueID class is designed to manage the lifecycle of HDF5 object handles (hid_t),
 * ensuring that the handle is properly closed when the UniqueID object is destroyed.
 * This helps prevent resource leaks and ensures proper cleanup of HDF5 resources.
 */
template <herr_t (*const D)(hid_t)> class UniqueID {
private:
  hid_t m_id;
  UniqueID(UniqueID<D> const &uid) = delete;
  UniqueID &operator=(UniqueID<D> const &) = delete;
  void close();

public:
  UniqueID(UniqueID<D> &&uid) noexcept : m_id(uid.m_id) { uid.m_id = INVALID_ID; }
  UniqueID<D> &operator=(hid_t const id);
  UniqueID<D> &operator=(UniqueID<D> &&uid);
  bool operator==(int const id) const { return static_cast<int>(m_id) == id; }
  bool operator<=(int const id) const { return static_cast<int>(m_id) <= id; }
  bool operator<(int const id) const { return static_cast<int>(m_id) < id; }
  operator hid_t() const { return m_id; }
  hid_t get() const { return m_id; }
  hid_t release();
  void reset(hid_t const id = INVALID_ID);
  bool isValid() const;
  UniqueID() : m_id(INVALID_ID) {}
  UniqueID(hid_t const id) : m_id(id) {}
  ~UniqueID() { close(); }

  static hid_t constexpr INVALID_ID{-1};
};

template <herr_t (*const D)(hid_t)> bool UniqueID<D>::isValid() const {
  // fail early condition
  if (m_id < 0) {
    return false;
  } else {
    return H5Iis_valid(m_id);
  }
}

template <herr_t (*const deleter)(hid_t)> void UniqueID<deleter>::close() {
  if (isValid()) {
    deleter(this->m_id);
    this->m_id = INVALID_ID;
  }
}

/// @brief  Release hold on the managed ID; it will not be closed by this UniqueID
/// @return the managed ID
template <herr_t (*const D)(hid_t)> hid_t UniqueID<D>::release() {
  hid_t tmp = m_id;
  m_id = INVALID_ID;
  return tmp;
}

template <herr_t (*const D)(hid_t)> void UniqueID<D>::reset(hid_t const id) {
  if (m_id != id) {
    close();
    m_id = id;
  }
}

/// @brief Assign a HDF5 object ID to be managed
/// @param id : the ID to be managed
template <herr_t (*const D)(hid_t)> UniqueID<D> &UniqueID<D>::operator=(hid_t const id) {
  reset(id);
  return *this;
}

/// @brief Pass the HDF5 object ID from an existing UniqueID to another UniqueID
/// @param uid : the UniqueID previously managing the ID; it will lose ownership of the ID.
template <herr_t (*const D)(hid_t)> UniqueID<D> &UniqueID<D>::operator=(UniqueID<D> &&uid) {
  if (this != &uid) {
    reset(uid.m_id);
    uid.m_id = INVALID_ID;
  }
  return *this;
}

} // namespace Mantid::Nexus
