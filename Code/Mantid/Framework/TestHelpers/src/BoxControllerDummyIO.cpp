/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This collection of functions MAY NOT be used in any test from a package
 *below
 *  API (e.g. Kernel, Geometry).
 *  Conversely, this file MAY NOT be modified to use anything from a package
 *higher
 *  than API (e.g. any algorithm or concrete workspace), even if via the
 *factory.
 *********************************************************************************/
#include "MantidTestHelpers/BoxControllerDummyIO.h"
#include "MantidKernel/Exception.h"

#include <string>

namespace MantidTestHelpers {
/**Constructor
 @param bc shared pointer to the box controller which will use this IO
 operations
*/
BoxControllerDummyIO::BoxControllerDummyIO(const Mantid::API::BoxController *bc)
    : m_bc(bc), m_CoordSize(4), m_TypeName("MDEvent"), m_ReadOnly(true),
      m_isOpened(false) {
  m_EventSize = static_cast<unsigned int>(bc->getNDims() + 4);
}

/**The optional method to set up the event type and the size of the event
coordinate
* As save/load operations use void data type, these function allow set up/get
the type name provided for the IO operations
*  and the size of the data type in bytes (e.g. the  class dependant physical
meaning of the blockSize and blockPosition used
*  by save/load operations
*@param blockSize -- size (in bytes) of the blockPosition and blockSize used in
save/load operations. 4 and 8 are supported only
                     (float and double)
*@param typeName  -- the name of the event used in the operations. The name
itself defines the size and the format of the event
                    The events described in the class header are supported only
*/
void BoxControllerDummyIO::setDataType(const size_t blockSize,
                                       const std::string &typeName) {
  if (blockSize == 4 || blockSize == 8) {
    m_CoordSize = static_cast<unsigned int>(blockSize);
  } else
    throw std::invalid_argument("The class currently supports 4(float) and "
                                "8(double) event coordinates only");
  m_TypeName = typeName;
  if (m_TypeName == "MDEvent") {
    m_EventSize = static_cast<unsigned int>(m_bc->getNDims() + 4);
  } else if (m_TypeName == "MDLeanEvent") {
    m_EventSize = static_cast<unsigned int>(m_bc->getNDims() + 2);
  } else {
    throw std::invalid_argument("unsupported event type");
  }
}

/** As save/load operations use void data type, these function allow set up/get
 *the type name provided for the IO operations
 *  and the size of the data type in bytes (e.g. the  class dependant physical
 *meaning of the blockSize and blockPosition used
 *  by save/load operations
 *@return CoordSize -- size (in bytes) of the blockPosition and blockSize used
 *in save/load operations
 *@return typeName  -- the name of the event used in the operations. The name
 *itself defines the size and the format of the event
*/

void BoxControllerDummyIO::getDataType(size_t &CoordSize,
                                       std::string &typeName) const {
  CoordSize = m_CoordSize;
  typeName = m_TypeName;
}

/**Open the file to use in IO operations with events
 *
 *@param fileName the name of the file to open.
 *                 if file name has word exist, the file is opened as existing
 *with 100 floats equal 2.
 *                 othewise if assumed to be new and size 0
 *@param mode  opening mode (read ("r" ) or read/write "w")
*/
bool BoxControllerDummyIO::openFile(const std::string &fileName,
                                    const std::string &mode) {
  m_fileName = fileName;
  // file already opened
  if (m_isOpened)
    return false;

  m_ReadOnly = true;
  ;
  if (mode.find("w") != std::string::npos ||
      mode.find("W") != std::string::npos) {
    m_ReadOnly = false;
  }

  // open file if it exists or crate it if not in the mode requested
  // bool fileExists(true);
  if (fileName.find("exist") != std::string::npos) {
    size_t nEvents = 1000;
    fileContents.assign(nEvents * m_EventSize, 0);
    this->setFileLength(nEvents);
    size_t ic(0);
    for (size_t i = 0; i < nEvents; i++) {
      fileContents[ic++] = static_cast<float>(i);
      fileContents[ic++] = static_cast<float>(i * i);
      for (size_t j = 2; j < m_EventSize; j++)
        fileContents[ic++] = static_cast<float>(i + 10 * j);
    }

  } else
    this->setFileLength(0);

  m_isOpened = true;

  return true;
}
/**Save block of data into properly opened and initiated direct access data file
 @param DataBlock     -- the vector with the data to write
 @param blockPosition -- the position of the data block within the data file
 itself
*/
void BoxControllerDummyIO::saveBlock(const std::vector<float> &DataBlock,
                                     const uint64_t blockPosition) const {
  size_t nEvents = DataBlock.size() / m_EventSize;
  uint64_t position = blockPosition;
  // uint64_t fileLength = this->getFileLength();
  m_fileMutex.lock();
  if (m_EventSize * (position + nEvents) > fileContents.size()) {
    fileContents.resize((position + nEvents) * m_EventSize);
    this->setFileLength(position + nEvents);
  }

  for (size_t i = 0; i < DataBlock.size(); i++) {
    fileContents[blockPosition * m_EventSize + i] = DataBlock[i];
  }
  m_fileMutex.unlock();
}
/**Load a block of data from properly prepared direct access data file
 @param Block        -- the vector for data to place into. If the size of the
 block is smaller then the requested size, the vector will be realocated.
                         The data are placed at the beginnign of the block.
 @param blockPosition -- the position of the data block within the data file
 @param nPoints       -- number of data points to read from the file. The
 datapoint size is defined when opened file or by calling the setDataType
 directrly

 *Throws if attempted to read data outside of the file.
*/
void BoxControllerDummyIO::loadBlock(std::vector<float> &Block,
                                     const uint64_t blockPosition,
                                     const size_t nPoints) const {
  Poco::ScopedLock<Mantid::Kernel::Mutex> _lock(m_fileMutex);
  if (blockPosition + nPoints > this->getFileLength())
    throw Mantid::Kernel::Exception::FileError(
        "Attemtp to read behind the file end", m_fileName);

  Block.resize(nPoints * m_EventSize);
  for (size_t i = 0; i < nPoints * m_EventSize; i++) {
    Block[i] = fileContents[blockPosition * m_EventSize + i];
  }
}

BoxControllerDummyIO::~BoxControllerDummyIO() { this->closeFile(); }
}
