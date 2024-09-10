// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include <list>
#include <mutex>
#ifndef Q_MOC_RUN
#include <optional>
#endif

namespace Mantid {
namespace Kernel {

/** An interface for objects that can be cached or saved to disk.
  This is implemented by MDBox and is used in the in-memory
  cache of file-backed MDEventWorkspaces.

  @author Janik Zikovsky
  @date 2011-07-28
*/
// forward declaration

class MANTID_KERNEL_DLL ISaveable {
public:
  ISaveable();
  ISaveable(const ISaveable &other);
  virtual ~ISaveable() = default;

  ///** @return the position in the file where the data will be stored. This is
  /// used to optimize file writing. */
  virtual uint64_t getFilePosition() const { return m_fileIndexStart; }
  /**Return the number of units this block occipies on file */
  uint64_t getFileSize() const { return m_fileNumEvents; }
  /** Sets the location of the object on HDD */
  void setFilePosition(uint64_t newPos, size_t newSize, bool wasSaved);
  //-----------------------------------------------------------------------------------------------
  // Saveable functions interface, which controls the logic of working with
  // objects on HDD

  /** @return true if the object have ever been saved on HDD and knows it place
   * there*/
  bool wasSaved() const // for speed it returns this boolean, but for
                        // relaibility this should be
                        // m_wasSaved&&(m_fileIndexStart!=max())
  {
    return m_wasSaved;
  }
  /**@return  true if the object has been load in the memory -- the load
   * function should call setter, or if the object was constructed in memory it
   * should be loaded too */
  bool isLoaded() const { return m_isLoaded; }

  // protected?
  /**sets the value of the isLoad parameter, indicating that data from HDD have
   *its image in memory
   *@param Yes -- boolean true or false --usually only load functiomn should set
   *it to true
   */
  void setLoaded(bool Yes) { m_isLoaded = Yes; }

  /// @return true if it the data of the object is busy and so cannot be
  /// cleared; false if the data was released and can be cleared/written.
  bool isBusy() const { return m_Busy; }
  /// @ set the data busy to prevent from removing them from memory. The process
  /// which does that should clean the data when finished with them
  void setBusy(bool On) { m_Busy = On; }

  // protected?

  /**@return the state of the parameter, which tells disk buffer to force
   * writing data
   * to disk despite the size of the object have not changed (so one have
   * probably done something with object contents. */
  bool isDataChanged() const { return m_dataChanged; }
  /** Call this method from the method which changes the object but keeps the
     object size the same to tell DiskBuffer to write it back
      the dataChanged ID is reset after save from the DataBuffer is emptied   */
  void setDataChanged() {
    if (this->wasSaved())
      m_dataChanged = true;
  }
  /** this method has to be called if the object has been discarded from memory
     and is not changed any more.
     It expected to be called from clearDataFromMemory. */
  void clearDataChanged() { m_dataChanged = false; }

  //-----------------------------------------------------------------------------------------------
  // INTERFACE:

  /// Save the data - to be overriden
  virtual void save() const = 0;

  /// Load the data - to be overriden
  virtual void load() = 0;

  /// Method to flush the data to disk and ensure it is written.
  virtual void flushData() const = 0;
  /// remove objects data from memory
  virtual void clearDataFromMemory() = 0;

  /** @return the amount of memory that the object takes as a whole.
      For filebased objects it should be the amount the object occupies in
     memory plus the size it occupies in file if the object has not been fully
     loaded
      or modified.
     * If the object has never been loaded, this should be equal to number of
     data points in the file
     */
  virtual uint64_t getTotalDataSize() const = 0;
  /// the data size kept in memory
  virtual size_t getDataMemorySize() const = 0;

protected:
  //--------------
  /// a user needs to set this variable to true preventing from deleting data
  /// from buffer
  bool m_Busy;
  /** a user needs to set this variable to true to allow DiskBuffer saving the
     object to HDD
      when it decides it suitable,  if the size of iSavable object in cache is
     unchanged from the previous
      save/load operation */
  bool m_dataChanged;
  // this tracks the history of operations, occuring over the data.
  /// this boolean indicates if the data were saved on HDD and have physical
  /// representation on it (though this representation may be incorrect as data
  /// changed in memory)
  mutable bool m_wasSaved;
  /// this boolean indicates, if the data have its copy in memory
  bool m_isLoaded;

private:
  // the iterator which describes the position of this object in the DiskBuffer.
  // Undefined if not placed to buffer
  std::optional<std::list<ISaveable *>::iterator> m_BufPosition;
  // the size of the object in the memory buffer, used to calculate the total
  // amount of memory the objects occupy
  size_t m_BufMemorySize;
  /// Start point in the NXS file where the events are located
  uint64_t m_fileIndexStart;
  /// Number of events saved in the file, after the start index location
  uint64_t m_fileNumEvents;

  /// the functions below have to be availible to DiskBuffer and nobody else. To
  /// highlight this we make them private
  friend class DiskBuffer;
  /** save at specific file location the specific amount of data;
       used by DiskBuffer which asks this object where to save it and calling
       overloaded object specific save operation above    */
  void saveAt(uint64_t newPos, uint64_t newSize);

  /// sets the iterator pointing to the location of this object in the memory
  /// buffer to write later
  size_t setBufferPosition(std::list<ISaveable *>::iterator bufPosition);
  /// returns the iterator pointing to the position of this object within the
  /// memory to-write buffer
  std::optional<std::list<ISaveable *>::iterator> &getBufPostion() { return m_BufPosition; }
  /// return the amount of memory, this object had when it was stored in buffer
  /// last time;
  size_t getBufferSize() const { return m_BufMemorySize; }
  void setBufferSize(size_t newSize) { m_BufMemorySize = newSize; }

  /// clears the state of the object, and indicate that it is not stored in
  /// buffer any more
  void clearBufferState();

  // the mutex to protect changes in this memory
  std::mutex m_setter;
};

} // namespace Kernel
} // namespace Mantid
