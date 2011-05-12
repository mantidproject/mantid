//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <limits>
#include <Poco/File.h>

namespace Mantid
{
namespace DataObjects
{

using std::size_t;

DECLARE_WORKSPACE(ManagedWorkspace2D)

// Initialise the instance count
int ManagedWorkspace2D::g_uniqueID = 1;

#ifdef _WIN32
#pragma warning (push)
#pragma warning( disable:4355 )
//Disable warning for using this in constructor.
//This is safe as long as the class that gets the pointer does not use any methods on it in it's constructor.
//In this case this is an inner class that is only used internally to this class, and is safe.
#endif //_WIN32
/// Constructor
ManagedWorkspace2D::ManagedWorkspace2D() :
  AbsManagedWorkspace2D(), m_indexWrittenTo(-1)
{
}
#ifdef _WIN32
#pragma warning (pop)
#endif //_WIN32

/** Sets the size of the workspace and sets up the temporary file
 *  @param NVectors :: The number of vectors/histograms/detectors in the workspace
 *  @param XLength :: The number of X data points/bin boundaries in each vector (must all be the same)
 *  @param YLength :: The number of data/error points in each vector (must all be the same)
 *  @throw std::runtime_error if unable to open a temporary file
 */
void ManagedWorkspace2D::init(const size_t &NVectors, const size_t &XLength, const size_t &YLength)
{
  m_noVectors = NVectors;
  m_axes.resize(2);
  m_axes[0] = new API::RefAxis(XLength, this);
  m_axes[1] = new API::SpectraAxis(NVectors);
  m_XLength = XLength;
  m_YLength = YLength;

  m_vectorSize = sizeof(int) + ( m_XLength + ( 2*m_YLength ) ) * sizeof(double);

  // CALCULATE BLOCKSIZE
  // Get memory size of a block from config file
  int blockMemory;
  if ( ! Kernel::ConfigService::Instance().getValue("ManagedWorkspace.DataBlockSize", blockMemory)
      || blockMemory <= 0 )
  {
    // default to 1MB if property not found
    blockMemory = 1024*1024;
  }


  m_vectorsPerBlock = blockMemory / static_cast<int>(m_vectorSize);
  // Should this ever come out to be zero, then actually set it to 1
  if ( m_vectorsPerBlock == 0 ) m_vectorsPerBlock = 1;
  
  //g_log.debug()<<"blockMemory: "<<blockMemory<<"\n";
  //g_log.debug()<<"m_vectorSize: "<<m_vectorSize<<"\n";
  //g_log.debug()<<"m_vectorsPerBlock: "<<m_vectorsPerBlock<<"\n";
  //g_log.debug()<<"Memeory: "<<getMemorySize()<<"\n";


  // Calculate the number of blocks that will go into a file
  m_blocksPerFile = std::numeric_limits<int>::max() / (m_vectorsPerBlock * static_cast<int>(m_vectorSize));
  if (std::numeric_limits<int>::max()%(m_vectorsPerBlock * m_vectorSize) != 0) ++m_blocksPerFile; 

  // Now work out the number of files needed
  int totalBlocks = m_noVectors / m_vectorsPerBlock;
  if (m_noVectors%m_vectorsPerBlock != 0) ++totalBlocks;
  int numberOfFiles = totalBlocks / m_blocksPerFile;
  if (totalBlocks%m_blocksPerFile != 0) ++numberOfFiles;
  m_datafile.resize(numberOfFiles);

  // Look for the (optional) path from the configuration file
  std::string path = Kernel::ConfigService::Instance().getString("ManagedWorkspace.FilePath");
  if( path.empty() || !Poco::File(path).exists() || !Poco::File(path).canWrite() )
  {
    path = Kernel::ConfigService::Instance().getUserPropertiesDir();
    g_log.debug() << "Temporary file written to " << path << std::endl;
  }
  // Append a slash if necessary
  if( ( *(path.rbegin()) != '/' ) && ( *(path.rbegin()) != '\\' ) )
  {
    path.push_back('/');
  }

  std::stringstream filestem;
  filestem << "WS2D" << ManagedWorkspace2D::g_uniqueID;
  m_filename = filestem.str() + this->getTitle() + ".tmp";
  // Increment the instance count
  ++ManagedWorkspace2D::g_uniqueID;
  std::string fullPath = path + m_filename;

  {
    std::string fileToOpen = fullPath + "0";

    // Create the temporary file
    m_datafile[0] = new std::fstream(fileToOpen.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

    if ( ! *m_datafile[0] )
    {
      m_datafile[0]->clear();
      // Try to open in current working directory instead
      std::string file = m_filename + "0";
      m_datafile[0]->open(file.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

      // Throw an exception if it still doesn't work
      if ( ! *m_datafile[0] )
      {
        g_log.error("Unable to open temporary data file");
        throw std::runtime_error("ManagedWorkspace2D: Unable to open temporary data file");
      }
    }
    else
    {
      m_filename = fullPath;
    }
  } // block to restrict scope of fileToOpen

  // Set exception flags for fstream so that any problems from now on will throw
  m_datafile[0]->exceptions( std::fstream::eofbit | std::fstream::failbit | std::fstream::badbit );

  // Open the other temporary files (if any)
  for (unsigned int i = 1; i < m_datafile.size(); ++i)
  {
    std::stringstream fileToOpen;
    fileToOpen << m_filename << i;
    m_datafile[i] = new std::fstream(fileToOpen.str().c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if ( ! *m_datafile[i] )
    {
      g_log.error("Unable to open temporary data file");
      throw std::runtime_error("ManagedWorkspace2D: Unable to open temporary data file");
    }
    m_datafile[i]->exceptions( std::fstream::eofbit | std::fstream::failbit | std::fstream::badbit );
  }

}

/// Destructor. Clears the buffer and deletes the temporary file.
ManagedWorkspace2D::~ManagedWorkspace2D()
{
  // delete all ManagedDataBlock2D's
  m_bufferedData.clear();
  // delete the temporary file and fstream objects
  for (unsigned int i = 0; i < m_datafile.size(); ++i)
  {
    m_datafile[i]->close();
    delete m_datafile[i];
    std::stringstream fileToRemove;
    fileToRemove << m_filename << i;
    remove(fileToRemove.str().c_str());
  }
}



/**  This function decides if ManagedDataBlock2D with given startIndex needs to 
     be loaded from storage and loads it.
     @param newBlock :: Returned data block address
     @param startIndex :: Starting spectrum index in the block
*/
void ManagedWorkspace2D::readDataBlock(ManagedDataBlock2D *newBlock,size_t startIndex)const
{
  // Check whether datablock has previously been saved. If so, read it in.
  if (startIndex <= m_indexWrittenTo)
  {
    long long seekPoint = startIndex * m_vectorSize;

    int fileIndex = 0;
    while (seekPoint > std::numeric_limits<int>::max())
    {
      seekPoint -= m_vectorSize * m_vectorsPerBlock * m_blocksPerFile;
      ++fileIndex;
    }

    // Safe to cast seekPoint to int because the while loop above guarantees that 
    // it'll be in range by this point.
    m_datafile[fileIndex]->seekg(static_cast<int>(seekPoint), std::ios::beg);
    *m_datafile[fileIndex] >> *newBlock;
  }

}

/**
 * Write a data block to disk.
 * @param toWrite :: pointer to the ManagedDataBlock2D to write.
 */
void ManagedWorkspace2D::writeDataBlock(ManagedDataBlock2D *toWrite) const
{
      int fileIndex = 0;
      // Check whether we need to pad file with zeroes before writing data
      if ( toWrite->minIndex() > m_indexWrittenTo+m_vectorsPerBlock && m_indexWrittenTo >= 0 )
      {
        fileIndex = m_indexWrittenTo / (m_vectorsPerBlock * m_blocksPerFile);

        m_datafile[fileIndex]->seekp(0, std::ios::end);
        const int speczero = 0;
        const std::vector<double> xzeroes(m_XLength);
        const std::vector<double> yzeroes(m_YLength);
        for (int i = 0; i < (toWrite->minIndex() - m_indexWrittenTo); ++i)
        {
          if ( (m_indexWrittenTo + i) / (m_blocksPerFile * m_vectorsPerBlock) )
          {
            ++fileIndex;
            m_datafile[fileIndex]->seekp(0, std::ios::beg);
          }

          m_datafile[fileIndex]->write((char *) &*xzeroes.begin(), m_XLength * sizeof(double));
          m_datafile[fileIndex]->write((char *) &*yzeroes.begin(), m_YLength * sizeof(double));
          m_datafile[fileIndex]->write((char *) &*yzeroes.begin(), m_YLength * sizeof(double));
          m_datafile[fileIndex]->write((char *) &*yzeroes.begin(), m_YLength * sizeof(double));
          m_datafile[fileIndex]->write((char *) &speczero, sizeof(int) );
        }
      }
      else
      // If no padding needed, go to correct place in file
      {
        long long seekPoint = toWrite->minIndex() * m_vectorSize;

        while (seekPoint > std::numeric_limits<int>::max())
        {
          seekPoint -= m_vectorSize * m_vectorsPerBlock * m_blocksPerFile;
          ++fileIndex;
        }

        // Safe to cast seekPoint to int because the while loop above guarantees that 
        // it'll be in range by this point.
        m_datafile[fileIndex]->seekp(static_cast<int>(seekPoint), std::ios::beg);
      }

      *m_datafile[fileIndex] << *toWrite;
      m_indexWrittenTo = std::max(m_indexWrittenTo, toWrite->minIndex());
}


/** Returns the number of histograms.
 *  For some reason Visual Studio couldn't deal with the main getHistogramNumber() method
 *  being virtual so it now just calls this private (and virtual) method which does the work.
 *  @return the number of histograms assocaited with the workspace
 */
size_t ManagedWorkspace2D::getHistogramNumberHelper() const
{
  return m_noVectors;
}

/// Return the size used in memory
size_t ManagedWorkspace2D::getMemorySize() const
{
    return size_t(m_vectorSize)*size_t(m_bufferedData.size())*size_t(m_vectorsPerBlock);
}

/// Return the full path to the file used.
std::string ManagedWorkspace2D::get_filename() const
{
  return this->m_filename;
}

} // namespace DataObjects
} // namespace Mantid
