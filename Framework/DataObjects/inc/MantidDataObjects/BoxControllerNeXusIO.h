// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxController.h"
#include "MantidAPI/IBoxControllerIO.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <mutex>

namespace Mantid {
namespace DataObjects {

//===============================================================================================
/** The class responsible for saving events into nexus file using generic box
  controller interface
  * Expected to provide thread-safe file access.

    @date March 15, 2013
*/
class MANTID_DATAOBJECTS_DLL BoxControllerNeXusIO : public API::IBoxControllerIO {
public:
  BoxControllerNeXusIO(API::BoxController *const bc);

  ///@return true if the file to write events is opened and false otherwise
  bool isOpened() const override { return m_File.get() != nullptr; }
  /// get the full file name of the file used for IO operations
  const std::string &getFileName() const override { return m_fileName; }
  /** Copy the file contents to a new location. */
  void copyFileTo(const std::string &destFilename) override;

  /**Return the size of the NeXus data block used in NeXus data array*/
  size_t getDataChunk() const override { return m_dataChunk; }

  bool openFile(const std::string &fileName, const std::string &mode) override;

  void saveBlock(const std::vector<float> & /* DataBlock */, const uint64_t /*blockPosition*/) const override;
  void loadBlock(std::vector<float> & /* Block */, const uint64_t /*blockPosition*/,
                 const size_t /*BlockSize*/) const override;
  void saveBlock(const std::vector<double> & /* DataBlock */, const uint64_t /*blockPosition*/) const override;
  void loadBlock(std::vector<double> & /* Block */, const uint64_t /*blockPosition*/,
                 const size_t /*BlockSize*/) const override;

  void flushData() const override;
  void closeFile() override;

  ~BoxControllerNeXusIO() override;
  // Auxiliary functions. Used to change default state of this object which is
  // not fully supported. Should be replaced by some IBoxControllerIO factory
  void setDataType(const size_t blockSize, const std::string &typeName) override;
  void getDataType(size_t &CoordSize, std::string &typeName) const override;
  //------------------------------------------------------------------------------------------------------------------------
  // Auxiliary functions (non-virtual, used for testing)
  int64_t getNDataColums() const { return m_BlockSize[1]; }
  // get pointer to the Nexus file --> compatribility testing only.
  ::NeXus::File *getFile() { return m_File.get(); }

  /**@brief The version of the "event_data" Nexus dataset
   *
   * @details The "event_data" Nexus dataset may contain all or only a subset
   * of the attributes of the current event object.
   *
   * | class      | attributes stored (besides coordinates)
   * -------------------------------------------------------
   *  MDLeanEvent | signal, error
   *  MDEvent     | signal, error, run-index, detector-index
   *  MDEvent     | signal, error, run-index, goniometer-index, detector-index
   *
   * The number of attributes stored (neglecting coordinates) define the version
   */
  enum class EventDataVersion : size_t { EDVLean = 2, EDVOriginal = 4, EDVGoniometer = 5 };

  EventDataVersion getEventDataVersion() const { return m_EventDataVersion; }

  void setEventDataVersion(const EventDataVersion &version);

  /**
   * @brief set the data version based on the number of attributes of the event,
   * not counting its coordinates.
   */
  void setEventDataVersion(const size_t &traitsCount);

  /**
   * @brief Number of data items in Nexus dataset "data_event" associated
   * with the particular event data version.
   */
  int64_t dataEventCount(void) const;

  /**
   * @brief Insert goniometer info in a block of event data, if necessary
   *
   * @details The dataset "event_data" in old Nexus files lack goniometer info,
   * thus it's necessary to insert the default goniometerIndex value into
   * a data-block that has been read from the file before it's consumed
   * by MDEvent::dataToEvents()
   *
   * @param Block : the storage vector containing the event data
   * @param accessMode : string specifying if we're reading from or writing to
   * file. Valid values are "READ" and "WRITE"
   */
  template <typename FloatOrDouble>
  void adjustEventDataBlock(std::vector<FloatOrDouble> &Block, const std::string &accessMode) const;

private:
  /// Default size of the events block which can be written in the NeXus array
  /// at once identified by efficiency or some other external reasons
  enum { DATA_CHUNK = 10000 };

  /// full file name (with path) of the Nexis file responsible for the IO
  /// operations (as NeXus filename has very strange properties and often
  /// truncated to 64 bytes)
  std::string m_fileName;
  /// the file Handler responsible for Nexus IO operations;
  std::unique_ptr<::NeXus::File> m_File;
  /// identifier if the file open only for reading or is  in read/write
  bool m_ReadOnly;
  /// The size of the events block which can be written in the neXus array at
  /// once (continuous part of the data block)
  size_t m_dataChunk;
  /// shared pointer to the box controller, which is repsoponsible for this IO
  API::BoxController *const m_bc;
  //------
  /// the start of the current data block to read from. It related to current
  /// physical representation of the data in NeXus file
  std::vector<int64_t> m_BlockStart;
  /// the vector, which describes the event specific data size, namely how many
  /// column an event is composed into and this class reads/writres
  std::vector<int64_t> m_BlockSize;
  /// lock Nexus file operations as Nexus is not thread safe
  mutable std::mutex m_fileMutex;

  // Mainly static information which may be split into different IO classes
  // selected through chein of responsibility.
  /// number of bytes in the event coordinates (coord_t length). Set by
  /// setDataType but can be defined statically with coord_t
  unsigned int m_CoordSize;
  /// possible event types this class understands. The enum numbers have to
  /// correspond to the numbers of symbolic event types,
  /// defined in EVENT_TYPES_SUPPORTED vector
  enum EventType {
    LeanEvent = 0, //< the event consisting of signal error and event coordinate
    FatEvent = 1   //< the event having the same as lean event plus RunID and detID
    /// the type of event (currently MD event or MDLean event this class deals
    /// with. )
  } m_EventType;

  /// The version of the md events data block
  std::string m_EventsVersion;

  /// "data_event" dataset version in the current Nexus file
  EventDataVersion m_EventDataVersion;

  /// the symbolic description of the event types currently supported by the
  /// class
  std::vector<std::string> m_EventsTypesSupported;
  /// data headers used for different events types
  std::vector<std::string> m_EventsTypeHeaders;

  /// the name of the Nexus data group for saving the events
  static std::string g_EventGroupName;
  /// the group name to save disk buffer data
  static std::string g_DBDataName;

  // helper functions:
  // prepare to write event nexus data in current data version format
  void CreateEventGroup();
  void OpenAndCheckEventGroup();
  void getDiskBufferFileData();
  void prepareNxSToWrite_CurVersion();
  void prepareNxSdata_CurVersion();
  // get the event type from event name
  static EventType TypeFromString(const std::vector<std::string> &typesSupported, const std::string &typeName);
  /// the enum, which suggests the way (currently)two possible data types are
  /// converted to each other
  enum CoordConversion {
    noConversion,
    floatToDouble,
    doubleToFolat
    /// conversion btween fload/double requested by the client
  } m_ReadConversion;

  template <typename Type>
  void saveGenericBlock(const std::vector<Type> &DataBlock, const uint64_t blockPosition) const;

  /// Load generic data block from the opened NeXus file.
  template <typename Type>
  void loadGenericBlock(std::vector<Type> &Block, const uint64_t blockPosition, const size_t nPoints) const;
};
} // namespace DataObjects
} // namespace Mantid
