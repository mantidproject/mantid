//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <limits>

DECLARE_WORKSPACE(ManagedWorkspace2D)

namespace Mantid
{
namespace DataObjects
{

// Initialise the instance count
int ManagedWorkspace2D::g_uniqueID = 1;

// Get a reference to the logger
Kernel::Logger& ManagedWorkspace2D::g_log = Kernel::Logger::get("ManagedWorkspace2D");

#ifdef _WIN32
#pragma warning (push)
#pragma warning( disable:4355 )
//Disable warning for using this in constructor.
//This is safe as long as the class that gets the pointer does not use any methods on it in it's constructor.
//In this case this is an inner class that is only used internally to this class, and is safe.
#endif //_WIN32
/// Constructor
ManagedWorkspace2D::ManagedWorkspace2D() :
  Workspace2D(), m_bufferedData(100, *this),m_indexWrittenTo(-1)
{
}
#ifdef _WIN32
#pragma warning (pop)
#endif //_WIN32

/** Sets the size of the workspace and sets up the temporary file
 *  @param NVectors The number of vectors/histograms/detectors in the workspace
 *  @param XLength The number of X data points/bin boundaries in each vector (must all be the same)
 *  @param YLength The number of data/error points in each vector (must all be the same)
 *  @throw std::runtime_error if unable to open a temporary file
 */
void ManagedWorkspace2D::init(const int &NVectors, const int &XLength, const int &YLength)
{
  m_noVectors = NVectors;
  m_axes.resize(2);
  m_axes[0] = new API::RefAxis(XLength, this);
  m_axes[1] = new API::Axis(API::AxisType::Spectra,NVectors);
  m_XLength = XLength;
  m_YLength = YLength;

  m_vectorSize = sizeof(int) + ( m_XLength + ( 3*m_YLength ) ) * sizeof(double);

  // CALCULATE BLOCKSIZE
  // Get memory size of a block from config file
  int blockMemory;
  if ( ! Kernel::ConfigService::Instance().getValue("ManagedWorkspace.DataBlockSize", blockMemory)
      || blockMemory <= 0 )
  {
    // default to 1MB if property not found
    blockMemory = 1024*1024;
  }

  m_vectorsPerBlock = blockMemory / m_vectorSize;
  // Should this ever come out to be zero, then actually set it to 1
  if ( m_vectorsPerBlock == 0 ) m_vectorsPerBlock = 1;

  // Calculate the number of blocks that will go into a file
  m_blocksPerFile = std::numeric_limits<int>::max() / (m_vectorsPerBlock * m_vectorSize);
  if (std::numeric_limits<int>::max()%(m_vectorsPerBlock * m_vectorSize) != 0) ++m_blocksPerFile;

  // Now work out the number of files needed
  int totalBlocks = m_noVectors / m_vectorsPerBlock;
  if (m_noVectors%m_vectorsPerBlock != 0) ++totalBlocks;
  int numberOfFiles = totalBlocks / m_blocksPerFile;
  if (totalBlocks%m_blocksPerFile != 0) ++numberOfFiles;
  m_datafile.resize(numberOfFiles);

  // Look for the (optional) path from the configuration file
  std::string path = Kernel::ConfigService::Instance().getString("ManagedWorkspace.FilePath");
  if ( !path.empty() )
  {
    if ( ( *(path.rbegin()) != '/' ) && ( *(path.rbegin()) != '\\' ) )
    {
      path.push_back('/');
    }
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

/// Get pseudo size
int ManagedWorkspace2D::size() const
{
  return m_noVectors * blocksize();
}

/// Get the size of each vector
int ManagedWorkspace2D::blocksize() const
{
  return (m_noVectors > 0) ? static_cast<int>(m_YLength) : 0;
}

/** Set the x values
 *  @param histnumber Index of the histogram to be set
 *  @param Vec The data to enter
 */
void ManagedWorkspace2D::setX(const int histnumber, const std::vector<double>& Vec)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setX, histogram number out of range");

  getDataBlock(histnumber)->setX(histnumber, Vec);
  return;
}

/** Set the x values
 *  @param histnumber Index of the histogram to be set
 *  @param PA The data to enter
 */
void ManagedWorkspace2D::setX(const int histnumber, const Histogram1D::RCtype& PA)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setX, histogram number out of range");

  getDataBlock(histnumber)->setX(histnumber, PA);
  return;
}

/** Set the x values
 *  @param histnumber Index of the histogram to be set
 *  @param Vec The data to enter
 */
void ManagedWorkspace2D::setX(const int histnumber, const Histogram1D::RCtype::ptr_type& Vec)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setX, histogram number out of range");

  getDataBlock(histnumber)->setX(histnumber, Vec);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param Vec The data to enter
 */
void ManagedWorkspace2D::setData(const int histnumber, const std::vector<double>& Vec)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setDAta, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, Vec);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param Vec The data to enter
 *  @param VecErr The corresponding errors
 */
void ManagedWorkspace2D::setData(const int histnumber, const std::vector<double>& Vec,
                                 const std::vector<double>& VecErr)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, Vec, VecErr);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param PY The data to enter
 */
void ManagedWorkspace2D::setData(const int histnumber, const Histogram1D::RCtype& PY)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param PY The data to enter
 *  @param PE The corresponding errors
 */
void ManagedWorkspace2D::setData(const int histnumber, const Histogram1D::RCtype& PY,
        const Histogram1D::RCtype& PE)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param PY The data to enter
 *  @param PE The corresponding errors
 */
void ManagedWorkspace2D::setData(const int histnumber, const Histogram1D::RCtype::ptr_type& PY,
        const Histogram1D::RCtype::ptr_type& PE)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE);
  return;
}

/** Get the x data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the x data
 */
std::vector<double>& ManagedWorkspace2D::dataX(const int index)
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::dataX, histogram number out of range");

  return getDataBlock(index)->dataX(index);
}

/** Get the y data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the y data
 */
std::vector<double>& ManagedWorkspace2D::dataY(const int index)
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::dataY, histogram number out of range");

  return getDataBlock(index)->dataY(index);
}

/** Get the error data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the error data
 */
std::vector<double>& ManagedWorkspace2D::dataE(const int index)
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::dataE, histogram number out of range");

  return getDataBlock(index)->dataE(index);
}

/** Get the x data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the x data
 */
const std::vector<double>& ManagedWorkspace2D::dataX(const int index) const
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::dataX, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataX(index);
}

/** Get the y data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the y data
 */
const std::vector<double>& ManagedWorkspace2D::dataY(const int index) const
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::dataY, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataY(index);
}

/** Get the error data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the error data
 */
const std::vector<double>& ManagedWorkspace2D::dataE(const int index) const
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::dataE, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataE(index);
}

///Returns the ErrorHelper applicable for this spectra
const API::IErrorHelper* ManagedWorkspace2D::errorHelper(const int index) const
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::errorHelper, histogram number out of range");

  return getDataBlock(index)->errorHelper(index);
}

///Sets the ErrorHelper for this spectra
void ManagedWorkspace2D::setErrorHelper(const int index,API::IErrorHelper* errorHelper)
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setErrorHelper, histogram number out of range");

  getDataBlock(index)->setErrorHelper(index,errorHelper);
}

///Sets the ErrorHelper for this spectra
void ManagedWorkspace2D::setErrorHelper(const int index,const API::IErrorHelper* errorHelper)
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("ManagedWorkspace2D::setErrorHelper, histogram number out of range");

  getDataBlock(index)->setErrorHelper(index,errorHelper);
}

/** Returns the number of histograms.
    For some reason Visual Studio couldn't deal with the main getHistogramNumber() method
	  being virtual so it now just calls this private (and virtual) method which does the work.
*/
const int ManagedWorkspace2D::getHistogramNumberHelper() const
{
  return m_noVectors;
}

/** Get a pointer to the data block containing the data corresponding to a given index
 *  @param index The index to search for
 *  @return A pointer to the data block containing the index requested
 */
// not really a const method, but need to pretend it is so that const data getters can call it
ManagedDataBlock2D* ManagedWorkspace2D::getDataBlock(const int index) const
{
  int startIndex = index - ( index%m_vectorsPerBlock );
  // Look to see if the data block is already buffered
  mru_list::const_iterator it = m_bufferedData.find(startIndex);
  if ( it != m_bufferedData.end() )
  {
    return *it;
  }

  // If not found, need to load block into memory and mru list
  ManagedDataBlock2D *newBlock = new ManagedDataBlock2D(startIndex, m_vectorsPerBlock, m_XLength, m_YLength);
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

    m_datafile[fileIndex]->seekg(seekPoint, std::ios::beg);
    *m_datafile[fileIndex] >> *newBlock;
  }

  m_bufferedData.insert(newBlock);
  return newBlock;
}

//----------------------------------------------------------------------
// mru_list member function definitions
//----------------------------------------------------------------------

/** Constructor
 *  @param max_num_items_ The length of the list
 *  @param out A reference to the containing class
 */
ManagedWorkspace2D::mru_list::mru_list(const std::size_t &max_num_items_, ManagedWorkspace2D &out) :
  max_num_items(max_num_items_),
  outer(out)
{
}

/** Insert an item into the list. If it's already in the list, it's moved to the top.
 *  If it's a new item, it's put at the top and the last item in the list is written to file and dropped.
 *  @param item The ManagedDataBlock to put in the list
 */
void ManagedWorkspace2D::mru_list::insert(ManagedDataBlock2D* item)
{
  std::pair<item_list::iterator,bool> p=il.push_front(item);

  if (!p.second)
  { /* duplicate item */
    il.relocate(il.begin(), p.first); /* put in front */
  }
  else if (il.size()>max_num_items)
  { /* keep the length <= max_num_items */
    // This is dropping an item - need to write it to disk (if it's changed) and delete
    ManagedDataBlock2D *toWrite = il.back();
    if ( toWrite->hasChanges() )
    {
      int fileIndex = 0;
      // Check whether we need to pad file with zeroes before writing data
      if ( toWrite->minIndex() > outer.m_indexWrittenTo+outer.m_vectorsPerBlock && outer.m_indexWrittenTo >= 0 )
      {
        fileIndex = outer.m_indexWrittenTo / (outer.m_vectorsPerBlock * outer.m_blocksPerFile);

        outer.m_datafile[fileIndex]->seekp(0, std::ios::end);
        const int speczero = 0;
        const std::vector<double> xzeroes(outer.m_XLength);
        const std::vector<double> yzeroes(outer.m_YLength);
        for (int i = 0; i < (toWrite->minIndex() - outer.m_indexWrittenTo); ++i)
        {
          if ( (outer.m_indexWrittenTo + i) / (outer.m_blocksPerFile * outer.m_vectorsPerBlock) )
          {
            ++fileIndex;
            outer.m_datafile[fileIndex]->seekp(0, std::ios::beg);
          }

          outer.m_datafile[fileIndex]->write((char *) &*xzeroes.begin(), outer.m_XLength * sizeof(double));
          outer.m_datafile[fileIndex]->write((char *) &*yzeroes.begin(), outer.m_YLength * sizeof(double));
          outer.m_datafile[fileIndex]->write((char *) &*yzeroes.begin(), outer.m_YLength * sizeof(double));
          outer.m_datafile[fileIndex]->write((char *) &*yzeroes.begin(), outer.m_YLength * sizeof(double));
          outer.m_datafile[fileIndex]->write((char *) &speczero, sizeof(int) );
        }
      }
      else
      // If no padding needed, go to correct place in file
      {
        long long seekPoint = toWrite->minIndex() * outer.m_vectorSize;

        while (seekPoint > std::numeric_limits<int>::max())
        {
          seekPoint -= outer.m_vectorSize * outer.m_vectorsPerBlock * outer.m_blocksPerFile;
          ++fileIndex;
        }

          outer.m_datafile[fileIndex]->seekp(seekPoint, std::ios::beg);
      }

      *outer.m_datafile[fileIndex] << *toWrite;
      outer.m_indexWrittenTo = std::max(outer.m_indexWrittenTo, toWrite->minIndex());
    }
    il.pop_back();
    delete toWrite;
  }
}

/// Delete all the data blocks pointed to by the list, and empty the list itself
void ManagedWorkspace2D::mru_list::clear()
{
  for (item_list::iterator it = il.begin(); it != il.end(); ++it)
  {
    delete *it;
  }
  il.clear();
}

} // namespace DataObjects
} // namespace Mantid
