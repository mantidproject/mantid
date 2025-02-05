// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/BoxControllerNeXusIO.h"

#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"

#include <H5Cpp.h>
#include <Poco/File.h>

#include <algorithm>
#include <string>

namespace Mantid::DataObjects {
// Default headers(attributes) describing the contents of the data, written by
// this class
const char *EventHeaders[] = {"signal, errorSquared, center (each dim.)",
                              "signal, errorSquared, expInfoIndex, goniometerIndex, detectorId, center (each "
                              "dim.)"};

std::string BoxControllerNeXusIO::g_EventGroupName("event_data");
std::string BoxControllerNeXusIO::g_DBDataName("free_space_blocks");

/**Constructor
 @param bc shared pointer to the box controller which uses this IO operations
*/
BoxControllerNeXusIO::BoxControllerNeXusIO(API::BoxController *const bc)
    : m_File(nullptr), m_ReadOnly(true), m_dataChunk(DATA_CHUNK), m_bc(bc), m_BlockStart(2, 0), m_BlockSize(2, 0),
      m_CoordSize(sizeof(coord_t)), m_EventType(FatEvent), m_EventsVersion("1.0"),
      m_EventDataVersion(EventDataVersion::EDVGoniometer), m_ReadConversion(noConversion) {
  m_BlockSize[1] = 5 + m_bc->getNDims();

  std::copy(std::cbegin(EventHeaders), std::cend(EventHeaders), std::back_inserter(m_EventsTypeHeaders));

  m_EventsTypesSupported.resize(2);
  m_EventsTypesSupported[LeanEvent] = MDLeanEvent<1>::getTypeName();
  m_EventsTypesSupported[FatEvent] = MDEvent<1>::getTypeName();
}
/**get event type form its string representation*/
BoxControllerNeXusIO::EventType BoxControllerNeXusIO::TypeFromString(const std::vector<std::string> &typesSupported,
                                                                     const std::string &typeName) {
  auto it = std::find(typesSupported.begin(), typesSupported.end(), typeName);
  if (it == typesSupported.end())
    throw std::invalid_argument("Unsupported event type: " + typeName + " provided ");

  return static_cast<EventType>(std::distance(typesSupported.begin(), it));
}
/** The optional method to set up the event type and the size of the event
  coordinate
  *  As save/load operations use void data type, these function allow set up/get
  the type name provided for the IO operations
  *  and the size of the data type in bytes (e.g. the  class dependent physical
  meaning of the blockSize and blockPosition used
  *  by save/load operations
  * @param blockSize -- size (in bytes) of the blockPosition and blockSize used
  in save/load operations. 4 and 8 are supported only
                        e.g. float and double
  * @param typeName  -- the name of the event used in the operations. The name
  itself defines the size and the format of the event
                        The events described in the class header are supported
  only  */
void BoxControllerNeXusIO::setDataType(const size_t blockSize, const std::string &typeName) {
  if (blockSize == 4 || blockSize == 8) {

    m_CoordSize = static_cast<unsigned int>(blockSize);
    m_EventType = TypeFromString(m_EventsTypesSupported, typeName);

    switch (m_EventType) {
    case (LeanEvent):
      m_BlockSize[1] = 2 + m_bc->getNDims();
      setEventDataVersion(EventDataVersion::EDVLean);
      break;
    case (FatEvent):
      m_BlockSize[1] = 5 + m_bc->getNDims();
      setEventDataVersion(EventDataVersion::EDVGoniometer);
      break;
    default:
      throw std::invalid_argument(" Unsupported event kind Identified  ");
    }
  } else
    throw std::invalid_argument("The class currently supports 4(float) and "
                                "8(double) event coordinates only");
}

/** As save/load operations use void data type, these function allow set up/get
 *the type name provided for the IO operations
 *  and the size of the data type in bytes (e.g. the  class dependent physical
 *meaning of the blockSize and blockPosition used
 *  by save/load operations
 *@param CoordSize -- size (in bytes) of the blockPosition and blockSize used
 *in save/load operations
 *@param typeName  -- the name of the event used in the operations. The name
 *itself defines the size and the format of the event
 */

void BoxControllerNeXusIO::getDataType(size_t &CoordSize, std::string &typeName) const {
  CoordSize = m_CoordSize;
  typeName = m_EventsTypesSupported[m_EventType];
}

/**Open the file to use in IO operations with events
 *
 *@param fileName -- the name of the file to open. Search for file performed
 *within the Mantid search path.
 *@param mode  -- opening mode (read or read/write)
 *
 *
 */
bool BoxControllerNeXusIO::openFile(const std::string &fileName, const std::string &mode) {
  // file already opened
  if (m_File)
    return false;

  std::lock_guard<std::mutex> _lock(m_fileMutex);
  m_ReadOnly = true;
  if (mode.find('w') != std::string::npos || mode.find('W') != std::string::npos) {
    m_ReadOnly = false;
  }

  // open file if it exists or create it if not in the mode requested
  m_fileName = API::FileFinder::Instance().getFullPath(fileName);
  if (m_fileName.empty()) {
    if (!m_ReadOnly) {
      std::string filePath = Kernel::ConfigService::Instance().getString("defaultsave.directory");
      if (filePath.empty())
        m_fileName = fileName;
      else
        m_fileName = filePath + "/" + fileName;
    } else
      throw Kernel::Exception::FileError("Can not open file to read ", m_fileName);
  }

  auto nDims = static_cast<int>(this->m_bc->getNDims());

  bool group_exists;
  m_File = std::unique_ptr<::NeXus::File>(MDBoxFlatTree::createOrOpenMDWSgroup(
      m_fileName, nDims, m_EventsTypesSupported[m_EventType], m_ReadOnly, group_exists));

  // we are in MD workspace Class  group now
  std::map<std::string, std::string> groupEntries;
  m_File->getEntries(groupEntries);
  if (groupEntries.find(g_EventGroupName) != groupEntries.end()) // yes, open it
    OpenAndCheckEventGroup();
  else // create and open it
    CreateEventGroup();
  // we are in MDEvent group now (either created or opened)

  // read if exist and create if not the group, which is responsible for saving
  // DiskBuffer information;
  getDiskBufferFileData();

  if (m_ReadOnly)
    prepareNxSdata_CurVersion();
  else
    prepareNxSToWrite_CurVersion();

  return true;
}

/**
 * Copy the underlying file to the given destination. If the file is opened and locked
 * then it is closed, the copy made and the file is reopened again with the same mode
 * @param destFilename A filepath to copy the file to.
 */
void BoxControllerNeXusIO::copyFileTo(const std::string &destFilename) {
  // Some OSs (observed on Windows) take an exclusive lock on the file
  // To copy the file must be closed, copied and reopened. To avoid
  // paying for this where not necessary first try without closing first
  try {
    Poco::File(this->getFileName()).copyTo(destFilename);
    return;
  } catch (const Poco::Exception &) {
    try {
      this->closeFile();
      Poco::File(this->getFileName()).copyTo(destFilename);
    } catch (...) {
      // if an exception happened during the copy attempt to reopen the original
      this->openFile(this->getFileName(), m_ReadOnly ? "r" : "w");
      throw;
    }
    this->openFile(this->getFileName(), m_ReadOnly ? "r" : "w");
  }
}

/**Create group responsible for keeping events and add necessary attributes to
 * it*/
void BoxControllerNeXusIO::CreateEventGroup() {
  if (m_ReadOnly)
    throw Kernel::Exception::FileError(
        "The NXdata group: " + g_EventGroupName + " does not exist in the file opened for read", m_fileName);

  try {
    m_File->makeGroup(g_EventGroupName, "NXdata", true);
    m_File->putAttr("version", m_EventsVersion);
  } catch (...) {
    throw Kernel::Exception::FileError("Can not create new NXdata group: " + g_EventGroupName, m_fileName);
  }
}

/** Open existing Event group and check the attributes necessary for this
 * algorithm to work */
void BoxControllerNeXusIO::OpenAndCheckEventGroup() {

  m_File->openGroup(g_EventGroupName, "NXdata");
  std::string fileGroupVersion;
  m_File->getAttr("version", fileGroupVersion);

  if (fileGroupVersion != m_EventsVersion)
    throw Kernel::Exception::FileError("Trying to open existing data grop to write new event data but the "
                                       "group with differetn version: " +
                                           fileGroupVersion + " already exists ",
                                       m_fileName);
}
/** Helper function which prepares NeXus event structure to accept events   */
void BoxControllerNeXusIO::prepareNxSToWrite_CurVersion() {

  // Are data already there?
  std::string EventData("event_data");
  std::map<std::string, std::string> groupEntries;
  m_File->getEntries(groupEntries);
  if (groupEntries.find(EventData) != groupEntries.end()) // yes, open it
  {
    prepareNxSdata_CurVersion();
  } else // no, create it
  {
    // Prepare the event data array for writing operations:
    m_BlockSize[0] = NX_UNLIMITED;

    // Now the chunk size.
    // m_Blocksize == (number_events_to_write_at_a_time, data_items_per_event)
    std::vector<int64_t> chunk(m_BlockSize);
    chunk[0] = static_cast<int64_t>(m_dataChunk);

    // Make and open the data
    if (m_CoordSize == 4)
      m_File->makeCompData("event_data", NXnumtype::FLOAT32, m_BlockSize, ::NeXus::NONE, chunk, true);
    else
      m_File->makeCompData("event_data", NXnumtype::FLOAT64, m_BlockSize, ::NeXus::NONE, chunk, true);

    // A little bit of description for humans to read later
    m_File->putAttr("description", m_EventsTypeHeaders[m_EventType]);
    // disk buffer knows that the file has no events
    this->setFileLength(0);
  }
}
/** Open the NXS data blocks for loading/saving.
 * The data should have been created before.     */
void BoxControllerNeXusIO::prepareNxSdata_CurVersion() {
  // Open the data
  m_File->openData("event_data");
  // There are rummors that this is faster. Not sure if it is important
  //      int type = NXnumtype::FLOAT32;
  //      int rank = 0;
  //      NXgetinfo(file->getHandle(), &rank, dims, &type);

  NeXus::Info info = m_File->getInfo();
  NXnumtype Type = info.type;

  m_ReadConversion = noConversion;
  switch (Type) {
  case (NXnumtype::FLOAT64):
    if (m_CoordSize == 4)
      m_ReadConversion = doubleToFolat;
    break;
  case (NXnumtype::FLOAT32):
    if (m_CoordSize == 8)
      m_ReadConversion = floatToDouble;
    break;

  default:
    throw Kernel::Exception::FileError("Unknown events data format ", m_fileName);
  }

  auto ndim2 = static_cast<size_t>(info.dims[1]); // number of columns
  setEventDataVersion(ndim2 - m_bc->getNDims());

  // HACK -- there is no difference between empty event dataset and the dataset
  // with 1 event.
  // It is unclear how to deal with this stuff but the situations, where the
  // dataset was created and closed without writing there anything
  // and then opened again to write data into it are probably rare.
  uint64_t nFilePoints = info.dims[0];
  this->setFileLength(nFilePoints);
}
/** Load free space blocks from the data file or create the NeXus place to
 * read/write them*/
void BoxControllerNeXusIO::getDiskBufferFileData() {
  std::vector<uint64_t> freeSpaceBlocks;
  this->getFreeSpaceVector(freeSpaceBlocks);
  if (freeSpaceBlocks.empty())
    freeSpaceBlocks.resize(2, 0); // Needs a minimum size

  //    // Get a vector of the free space blocks to save to the file
  std::vector<int64_t> free_dims(2, 2);
  free_dims[0] = int64_t(freeSpaceBlocks.size() / 2);
  std::vector<int64_t> free_chunk(2, 2);
  free_chunk[0] = int64_t(m_dataChunk);

  std::map<std::string, std::string> groupEntries;
  m_File->getEntries(groupEntries);
  if (groupEntries.find(g_DBDataName) != groupEntries.end()) // data exist, open it
  {
    // Read the free space blocks in from the existing file
    m_File->readData(g_DBDataName, freeSpaceBlocks);
    this->setFreeSpaceVector(freeSpaceBlocks);
  } else // create and open the group
  {
    if (m_ReadOnly)
      throw Kernel::Exception::FileError("Attempt to create new DB group in the read-only file", m_fileName);
    m_File->writeExtendibleData(g_DBDataName, freeSpaceBlocks, free_dims, free_chunk);
  }
}

//-------------------------------------------------------------------------------------------------------------------------------------
/** Save generc data block on specific position within properly opened NeXus
 *data array
 *@param DataBlock     -- the vector with data to write
 *@param blockPosition -- The starting place to save data to   */
template <typename Type>
void BoxControllerNeXusIO::saveGenericBlock(const std::vector<Type> &DataBlock, const uint64_t blockPosition) const {
  std::vector<int64_t> start(2, 0);
  // Specify the dimensions
  std::vector<int64_t> dims(m_BlockSize);

  std::lock_guard<std::mutex> _lock(m_fileMutex);
  start[0] = int64_t(blockPosition);
  dims[0] = int64_t(DataBlock.size() / this->getNDataColums());

  // ugly cast but why would putSlab change the data?. This is NeXus bug which
  // makes putSlab method non-constant
  auto &mData = const_cast<std::vector<Type> &>(DataBlock);

  {
    m_File->putSlab<Type>(mData, start, dims);

    if (blockPosition + dims[0] > this->getFileLength())
      this->setFileLength(blockPosition + dims[0]);
  }
}

/** Save float data block on specific position within properly opened NeXus data
 *array
 *@param DataBlock     -- the vector with data to write
 *@param blockPosition -- The starting place to save data to   */
void BoxControllerNeXusIO::saveBlock(const std::vector<float> &DataBlock, const uint64_t blockPosition) const {
  this->saveGenericBlock(DataBlock, blockPosition);
}
/** Save double precision data block on specific position within properly opened
 *NeXus data array
 *@param DataBlock     -- the vector with data to write
 *@param blockPosition -- The starting place to save data to   */
void BoxControllerNeXusIO::saveBlock(const std::vector<double> &DataBlock, const uint64_t blockPosition) const {
  this->saveGenericBlock(DataBlock, blockPosition);
}

void BoxControllerNeXusIO::setEventDataVersion(const BoxControllerNeXusIO::EventDataVersion &version) {
  using EDV = BoxControllerNeXusIO::EventDataVersion;

  // Is the event of type MDLeanEvent of MDEvent
  size_t coordSize;
  std::string typeName;
  getDataType(coordSize, typeName);

  // Cross-validation implemented in a not very elegant way, but does the job
  // MDLeanEvent only accepts EDVLean
  if (typeName == MDLeanEvent<1>::getTypeName() && version != EDV::EDVLean)
    throw std::invalid_argument("Cannot set the event data version to other than EDVLean");
  // MDEvent cannot accept EDVLean
  if (typeName == MDEvent<1>::getTypeName() && version == EDV::EDVLean)
    throw std::invalid_argument("Cannot set the event data version to EDVLean");

  m_EventDataVersion = version;
}

void BoxControllerNeXusIO::setEventDataVersion(const size_t &traitsCount) {
  using EDV = EventDataVersion;
  auto edv = static_cast<EventDataVersion>(traitsCount);
  // sucks I couldn't create this list dynamically
  std::initializer_list<EDV> valid_versions = {EDV::EDVLean, EDV::EDVOriginal, EDV::EDVGoniometer};
  const auto valid_edv = std::find(std::cbegin(valid_versions), std::cend(valid_versions), edv);
  if (valid_edv != std::cend(valid_versions)) {
    setEventDataVersion(*valid_edv);
    return;
  } else {
    // should not reach here
    throw std::invalid_argument("Could not find a valid version");
  }
}

int64_t BoxControllerNeXusIO::dataEventCount(void) const {
  // m_BlockSize[1] is the number of data events associated to an MDLeanEvent
  // or MDEvent object.
  int64_t size(m_BlockSize[1]);
  switch (m_EventDataVersion) {
  case (EventDataVersion::EDVLean):
    break;                              // no adjusting is necessary
  case (EventDataVersion::EDVOriginal): // we're dealing with an old Nexus file
    size -= 1;                          // old Nexus file doesn't have goniometer index info
    break;
  case (EventDataVersion::EDVGoniometer):
    break; // no adjusting is necessary
  default:
    throw std::runtime_error("Unknown event data version");
  }
  return size;
}

template <typename FloatOrDouble>
void BoxControllerNeXusIO::adjustEventDataBlock(std::vector<FloatOrDouble> &Block,
                                                const std::string &accessMode) const {
  // check the validity of accessMode
  const std::vector<std::string> validAccessModes{"READ", "WRITE"};
  if (std::find(validAccessModes.begin(), validAccessModes.end(), accessMode) == validAccessModes.end())
    throw std::runtime_error("Unknown access mode");

  switch (m_EventDataVersion) {
  case (EventDataVersion::EDVLean):
    break; // no adjusting is necessary
  // we're dealing with and old Nexus file
  case (EventDataVersion::EDVOriginal): {
    size_t blockSize(static_cast<size_t>(Block.size()));
    // number of data items in the old Nexus file associated to a single event
    size_t eventSize(dataEventCount());

    // we just read event data from an old Nexus file. Goniometer index must
    // be inserted
    if (accessMode == "READ") {
      // Block size is a multiple of eventSize
      size_t eventCount = blockSize / eventSize;
      if (eventCount * eventSize != blockSize) // this shouldn't happen
        throw std::runtime_error("Data block does not represent an integer number of events");
      // Loop to insert goniometer index
      std::vector<FloatOrDouble> backupBlock(Block);
      Block.resize(eventCount * (eventSize + 1));
      size_t blockCounter(0);
      size_t backupBlockCounter(0);
      for (size_t i = 0; i < eventCount; i++) {
        // signal, error, and expInfoIndex occupy the first three data items
        for (size_t j = 0; j < 3; j++) {
          Block[blockCounter] = backupBlock[backupBlockCounter];
          blockCounter++;
          backupBlockCounter++;
        }
        Block[blockCounter] = 0; // here's the goniometer index!
        blockCounter++;
        for (size_t j = 3; j < eventSize; j++) {
          Block[blockCounter] = backupBlock[backupBlockCounter];
          blockCounter++;
          backupBlockCounter++;
        }
      }
    }
    // we want to write the Block to an old Nexus file. Goniometer info must
    // be removed
    else if (accessMode == "WRITE") {
      // Block size is a multiple of (eventSize + 1)
      size_t eventCount = blockSize / (eventSize + 1);
      if (eventCount * (eventSize + 1) != blockSize) // this shouldn't happen
        throw std::runtime_error("Data block does not represent an integer number of events");
      // Loop to remove goniometer index
      std::vector<FloatOrDouble> backupBlock(Block);
      Block.resize(eventCount * eventSize);
      size_t blockCounter(0);
      size_t backupBlockCounter(0);
      for (size_t i = 0; i < eventCount; i++) {
        // signal, error, and expInfoIndex occupy the first three data items
        for (size_t j = 0; j < 3; j++) {
          Block[blockCounter] = backupBlock[backupBlockCounter];
          blockCounter++;
          backupBlockCounter++;
        }
        backupBlockCounter++; // skip the goniometer index
        for (size_t j = 3; j < eventSize; j++) {
          Block[blockCounter] = backupBlock[backupBlockCounter];
          blockCounter++;
          backupBlockCounter++;
        }
      }
    }
    break;
  }
  case (EventDataVersion::EDVGoniometer):
    break; // no adjusting is necessary
  default:
    throw std::runtime_error("Unknown event data version");
  }
}

// explicit instantiations
template DLLExport void BoxControllerNeXusIO::adjustEventDataBlock<float>(std::vector<float> &Block,
                                                                          const std::string &accessMode) const;
template DLLExport void BoxControllerNeXusIO::adjustEventDataBlock<double>(std::vector<double> &Block,
                                                                           const std::string &accessMode) const;

template <typename Type>
void BoxControllerNeXusIO::loadGenericBlock(std::vector<Type> &Block, const uint64_t blockPosition,
                                            const size_t nPoints) const {
  if (blockPosition + nPoints > this->getFileLength())
    throw Kernel::Exception::FileError("Attemtp to read behind the file end", m_fileName);

  std::vector<int64_t> start(2, 0);
  start[0] = static_cast<int64_t>(blockPosition);

  std::vector<int64_t> size(m_BlockSize);
  size[0] = static_cast<int64_t>(nPoints);
  size[1] = dataEventCount(); // data item count per event in the Nexus file

  std::lock_guard<std::mutex> _lock(m_fileMutex);

  Block.resize(size[0] * size[1]);
  m_File->getSlab(&Block[0], start, size);

  adjustEventDataBlock(Block, "READ"); // insert goniometer info if necessary
}

/** Helper funcion which allows to convert one data fomat into another */
template <typename FROM, typename TO> void convertFormats(const std::vector<FROM> &inData, std::vector<TO> &outData) {
  outData.reserve(inData.size());
  for (size_t i = 0; i < inData.size(); i++) {
    outData.emplace_back(static_cast<TO>(inData[i]));
  }
}
/** Load float  data block from the opened NeXus file.
 *@param Block         -- the storage vector to place data into
 *@param blockPosition -- The starting place to read data from
 *@param nPoints       -- number of data points (events) to read
 */
void BoxControllerNeXusIO::loadBlock(std::vector<float> &Block, const uint64_t blockPosition,
                                     const size_t nPoints) const {
  std::vector<double> tmp;
  switch (m_ReadConversion) {
  case (noConversion):
    loadGenericBlock(Block, blockPosition, nPoints);
    break;
  case (doubleToFolat):
    loadGenericBlock(tmp, blockPosition, nPoints);
    convertFormats(tmp, Block);
    break;
  default:
    throw Kernel::Exception::FileError(" Attempt to read float data from unsupported file format", m_fileName);
  }
}
/** Load double  data block from the opened NeXus file.
 *@param Block         -- the storage vector to place data into
 *@param blockPosition -- The starting place to read data from
 *@param nPoints       -- number of data points (events) to read
 */
void BoxControllerNeXusIO::loadBlock(std::vector<double> &Block, const uint64_t blockPosition,
                                     const size_t nPoints) const {
  std::vector<float> tmp;
  switch (m_ReadConversion) {
  case (noConversion):
    loadGenericBlock(Block, blockPosition, nPoints);
    break;
  case (floatToDouble):
    loadGenericBlock(tmp, blockPosition, nPoints);
    convertFormats(tmp, Block);
    break;
  default:
    throw Kernel::Exception::FileError(" Attempt to read double data from unsupported file format", m_fileName);
  }
}

//-------------------------------------------------------------------------------------------------------------------------------------

/// Clear NeXus internal cache
void BoxControllerNeXusIO::flushData() const {
  std::lock_guard<std::mutex> _lock(m_fileMutex);
  m_File->flush();
}
/** flush disk buffer data from memory and close underlying NeXus file*/
void BoxControllerNeXusIO::closeFile() {
  if (m_File) {
    // write all file-backed data still stack in the data buffer into the file.
    this->flushCache();
    // lock file
    std::lock_guard<std::mutex> _lock(m_fileMutex);

    m_File->closeData(); // close events data
    if (!m_ReadOnly)     // write free space groups from the disk buffer
    {
      std::vector<uint64_t> freeSpaceBlocks;
      this->getFreeSpaceVector(freeSpaceBlocks);
      if (!freeSpaceBlocks.empty()) {
        std::vector<int64_t> free_dims(2, 2);
        free_dims[0] = int64_t(freeSpaceBlocks.size() / 2);

        m_File->writeUpdatedData(g_DBDataName, freeSpaceBlocks, free_dims);
      }
    }

    m_File->closeGroup(); // close events group
    m_File->closeGroup(); // close workspace group
    m_File->close();      // close NeXus file
    m_File = nullptr;
  }
}

BoxControllerNeXusIO::~BoxControllerNeXusIO() { this->closeFile(); }
} // namespace Mantid::DataObjects
