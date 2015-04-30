#ifndef MANTID_DATAOBJECTS_BOXCONTROLLER_NEXUSS_IO_H
#define MANTID_DATAOBJECTS_BOXCONTROLLER_NEXUSS_IO_H

#include "MantidAPI/IBoxControllerIO.h"
#include "MantidAPI/BoxController.h"
#include "MantidKernel/DiskBuffer.h"
#include <Poco/Mutex.h>
#include <nexus/NeXusFile.hpp>

namespace Mantid {
namespace DataObjects {

//===============================================================================================
/** The class responsible for saving events into nexus file using generic box
  controller interface
  * Expected to provide thread-safe file access.

    @date March 15, 2013

    Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
  Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport BoxControllerNeXusIO : public API::IBoxControllerIO {
public:
  BoxControllerNeXusIO(API::BoxController *const theBC);

  ///@return true if the file to write events is opened and false otherwise
  virtual bool isOpened() const { return (m_File != NULL); }
  /// get the full file name of the file used for IO operations
  virtual const std::string &getFileName() const { return m_fileName; }
  /**Return the size of the NeXus data block used in NeXus data array*/
  size_t getDataChunk() const { return m_dataChunk; }

  virtual bool openFile(const std::string &fileName, const std::string &mode);

  virtual void saveBlock(const std::vector<float> & /* DataBlock */,
                         const uint64_t /*blockPosition*/) const;
  virtual void loadBlock(std::vector<float> & /* Block */,
                         const uint64_t /*blockPosition*/,
                         const size_t /*BlockSize*/) const;
  virtual void saveBlock(const std::vector<double> & /* DataBlock */,
                         const uint64_t /*blockPosition*/) const;
  virtual void loadBlock(std::vector<double> & /* Block */,
                         const uint64_t /*blockPosition*/,
                         const size_t /*BlockSize*/) const;

  virtual void flushData() const;
  virtual void closeFile();

  virtual ~BoxControllerNeXusIO();
  // Auxiliary functions. Used to change default state of this object which is
  // not fully supported. Should be replaced by some IBoxControllerIO factory
  virtual void setDataType(const size_t coordSize, const std::string &typeName);
  virtual void getDataType(size_t &coordSize, std::string &typeName) const;
  //------------------------------------------------------------------------------------------------------------------------
  // Auxiliary functions (non-virtual, used for testing)
  int64_t getNDataColums() const { return m_BlockSize[1]; }
  // get pointer to the Nexus file --> compatribility testing only.
  ::NeXus::File *getFile() { return m_File; }

private:
  /// Default size of the events block which can be written in the NeXus array
  /// at once identified by efficiency or some other external reasons
  enum { DATA_CHUNK = 10000 };

  /// full file name (with path) of the Nexis file responsible for the IO
  /// operations (as NeXus filename has very strange properties and often
  /// truncated to 64 bytes)
  std::string m_fileName;
  /// the file Handler responsible for Nexus IO operations;
  ::NeXus::File *m_File;
  /// identifier if the file open only for reading or is  in read/write
  bool m_ReadOnly;
  /// The size of the events block which can be written in the neXus array at
  /// once (continious part of the data block)
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
  /// lock Nexus file operations as Nexus is not thread safe // Poco::Mutex -- >
  /// is recursive.
  mutable Poco::FastMutex m_fileMutex;

  // Mainly static information which may be split into different IO classes
  // selected through chein of responsibility.
  /// number of bytes in the event coorinates (coord_t length). Set by
  /// setDataType but can be defined statically with coord_t
  unsigned int m_CoordSize;
  /// possible event types this class understands. The enum numbers have to
  /// correspond to the numbers of symbolic event types,
  /// defined in EVENT_TYPES_SUPPORTED vector
  enum EventType {
    LeanEvent = 0, //< the event consisting of signal error and event coordinate
    FatEvent =
        1 //< the event having the same as lean event plus RunID and detID
    /// the type of event (currently MD event or MDLean event this class deals
    /// with. )
  } m_EventType;

  /// The version of the md events data block
  std::string m_EventsVersion;
  /// the symblolic description of the event types currently supported by the
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
  static EventType
  TypeFromString(const std::vector<std::string> &typesSupported,
                 const std::string typeName);
  /// the enum, which suggests the way (currently)two possible data types are
  /// converted to each other
  enum CoordConversion {
    noConversion,
    floatToDouble,
    doubleToFolat
    /// conversion btween fload/double requested by the client
  } m_ReadConversion;

  template <typename Type>
  void saveGenericBlock(const std::vector<Type> &DataBlock,
                        const uint64_t blockPosition) const;
  template <typename Type>
  void loadGenericBlock(std::vector<Type> &DataBlock,
                        const uint64_t blockPosition,
                        const size_t blockSize) const;
};
}
}
#endif
