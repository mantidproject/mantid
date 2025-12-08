#include "MantidNexus/NexusFile_fwd.h"
#include <memory>

// external forward declare
extern "C" herr_t H5Iis_valid(hid_t);

namespace Mantid::Nexus {

/**
 * \class UniqueID
 * \brief A wrapper class for managing HDF5 object handles (hid_t).
 *
 * The UniqueID class is designed to manage the lifecycle of HDF5 object handles (hid_t),
 * ensuring that the handle is properly closed when the UniqueID object is destroyed.
 * This helps prevent resource leaks and ensures proper cleanup of HDF5 resources.
 */
template <herr_t (*const deleter)(hid_t)> class UniqueID {
private:
  hid_t m_id;
  UniqueID &operator=(UniqueID<deleter> const &) = delete;
  UniqueID &operator=(UniqueID<deleter> const &&) = delete;
  virtual void closeId();

public:
  UniqueID(UniqueID<deleter> &uid) noexcept : m_id(uid.m_id) { uid.m_id = INVALID_ID; };
  UniqueID(UniqueID<deleter> &&uid) noexcept : m_id(uid.m_id) { uid.m_id = INVALID_ID; };
  virtual UniqueID &operator=(hid_t const id);
  virtual UniqueID &operator=(UniqueID<deleter> &uid);
  virtual bool operator==(int const id) const { return static_cast<int>(m_id) == id; }
  virtual bool operator<=(int const id) const { return static_cast<int>(m_id) <= id; }
  virtual bool operator<(int const id) const { return static_cast<int>(m_id) < id; }
  virtual operator hid_t const &() const { return m_id; };
  virtual hid_t getId() const { return m_id; }
  virtual hid_t releaseId();
  virtual void resetId(hid_t const id = INVALID_ID);
  virtual bool isValid() const;
  UniqueID() : m_id(INVALID_ID) {}
  UniqueID(hid_t const id) : m_id(id) {}
  virtual ~UniqueID() { closeId(); }

  static hid_t constexpr INVALID_ID{-1};
};

template <herr_t (*const D)(hid_t)> bool UniqueID<D>::isValid() const { return H5Iis_valid(m_id); }

template <herr_t (*const deleter)(hid_t)> void UniqueID<deleter>::closeId() {
  if (isValid()) {
    deleter(this->m_id);
    this->m_id = INVALID_ID;
  }
}

/// @brief  Release hold on the managed ID; it will not be closed by this UniqueID
/// @return the managed ID
template <herr_t (*const D)(hid_t)> hid_t UniqueID<D>::releaseId() {
  hid_t tmp = m_id;
  m_id = INVALID_ID;
  return tmp;
}

template <herr_t (*const D)(hid_t)> void UniqueID<D>::resetId(hid_t const id) {
  if (m_id != id) {
    closeId();
    m_id = id;
  }
}

/// @brief Assign a HDF5 object ID to be managed
/// @param id : the ID to be managed
template <herr_t (*const D)(hid_t)> UniqueID<D> &UniqueID<D>::operator=(hid_t const id) {
  resetId(id);
  return *this;
}

/// @brief Pass the HDF5 object ID from an existing UniqueID to another UniqueID
/// @param uid : the UniqueID previously managing the ID; it will lose ownership of the ID.
template <herr_t (*const D)(hid_t)> UniqueID<D> &UniqueID<D>::operator=(UniqueID<D> &uid) {
  resetId(uid.m_id);
  uid.m_id = INVALID_ID;
  return *this;
}

} // namespace Mantid::Nexus
