// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ISaveable.h"
//#include "MantidKernel/INode.h"

namespace Mantid::Kernel {

/** Constructor    */
ISaveable::ISaveable()
    : m_Busy(false), m_dataChanged(false), m_wasSaved(false), m_isLoaded(false), m_BufMemorySize(0),
      m_fileIndexStart(std::numeric_limits<uint64_t>::max()), m_fileNumEvents(0) {}

//----------------------------------------------------------------------------------------------
/** Copy constructor --> needed for std containers and not to copy mutexes
    Note setting isLoaded to false to break connection with the file object
   which is not copyale */
ISaveable::ISaveable(const ISaveable &other)
    : m_Busy(other.m_Busy), m_dataChanged(other.m_dataChanged), m_wasSaved(other.m_wasSaved), m_isLoaded(false),
      m_BufPosition(other.m_BufPosition), m_BufMemorySize(other.m_BufMemorySize),
      m_fileIndexStart(other.m_fileIndexStart), m_fileNumEvents(other.m_fileNumEvents)

{}

//---------------------------------------------------------------------------

/** Set the start/end point in the file where the events are located
 * @param newPos :: start point,
 * @param newSize :: number of events in the file
 * @param wasSaved :: flag to mark if the info was saved, by default it does
 */
void ISaveable::setFilePosition(uint64_t newPos, size_t newSize, bool wasSaved) {
  std::lock_guard<std::mutex> lock(m_setter);
  this->m_fileIndexStart = newPos;
  this->m_fileNumEvents = static_cast<uint64_t>(newSize);
  m_wasSaved = wasSaved;
}

// ----------- PRIVATE, only DB availible

/** private function which used by the disk buffer to save the contents of the
 object
 @param newPos -- new position to save object to
 @param newSize -- new size of the saveable object
*/
void ISaveable::saveAt(uint64_t newPos, uint64_t newSize) {
  std::lock_guard<std::mutex> lock(m_setter);
  // load old contents if it was there
  if (this->wasSaved())
    this->load();
  // set new position, derived by the disk buffer
  m_fileIndexStart = newPos;
  m_fileNumEvents = newSize;
  // save in the new location
  this->save();
  this->clearDataFromMemory();
}

/** Method stores the position of the object in Disc buffer and returns the size
 * of this object for disk buffer to store
 * @param bufPosition -- the allocator which specifies the position of the
 * object in the list of objects to write
 * @returns the size of the object it currently occupies in memory. This size is
 * also stored by the object itself for further references
 */
size_t ISaveable::setBufferPosition(std::list<ISaveable *>::iterator bufPosition) {
  std::lock_guard<std::mutex> lock(m_setter);
  m_BufPosition = std::optional<std::list<ISaveable *>::iterator>(bufPosition);
  m_BufMemorySize = this->getDataMemorySize();

  return m_BufMemorySize;
}

/// clears the state of the object, and indicate that it is not stored in buffer
/// any more
void ISaveable::clearBufferState() {
  std::lock_guard<std::mutex> lock(m_setter);

  m_BufMemorySize = 0;
  m_BufPosition = std::optional<std::list<ISaveable *>::iterator>();
}

} // namespace Mantid::Kernel
