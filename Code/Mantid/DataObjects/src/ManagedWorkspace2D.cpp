//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidKernel/ConfigService.h"

DECLARE_WORKSPACE(ManagedWorkspace2D)

namespace Mantid
{
namespace DataObjects
{

// Get a reference to the logger
Kernel::Logger& ManagedWorkspace2D::g_log = Kernel::Logger::get("ManagedWorkspace2D");

/// Constructor
ManagedWorkspace2D::ManagedWorkspace2D() :
  Workspace2D(), m_bufferedData(100, *this)
{}

/** Sets the size of the workspace and sets up the temporary file
 *  @param NVectors The number of vectors/histograms/detectors in the workspace
 *  @param XLength The number of X data points/bin boundaries in each vector (must all be the same)
 *  @param YLength The number of data/error points in each vector (must all be the same)
 */
void ManagedWorkspace2D::init(const unsigned int &NVectors, const unsigned int &XLength, const unsigned int &YLength)
{
  m_noVectors = NVectors;
  m_XLength = XLength;
  m_YLength = YLength;
  
  // should also get path to put file in out of properties
  m_filename = this->getTitle() + ".tmp";
  // Create the temporary file
  m_datafile.open(m_filename.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
  if ( ! m_datafile )
  {
    g_log.error("Unable to open temporary data file.");
    // This would be a big problem - probably need to throw
  }
  // Set exception flags for fstream so that any problems will throw
  m_datafile.exceptions( std::fstream::eofbit | std::fstream::failbit | std::fstream::badbit );
  
  // CALCULATE BLOCKSIZE
  // Get memory size of a block from config file
  int blockMemory;
  if ( ! Kernel::ConfigService::Instance()->getValue("ManagedWorkspace.DataBlockSize", blockMemory) 
      || blockMemory <= 0 )
  {
    // default to 1MB if property not found
    blockMemory = 1024*1024;
  }

  m_vectorsPerBlock = blockMemory / ( ( m_XLength + ( 3*m_YLength ) ) * sizeof(double) );
  // Should this ever come out to be zero, then actually set it to 1
  if ( m_vectorsPerBlock == 0 ) m_vectorsPerBlock = 1;
  unsigned int numberOfBlocks = m_noVectors / m_vectorsPerBlock;
  if ( m_noVectors%m_vectorsPerBlock != 0 ) ++numberOfBlocks;

  // Fill the temporary file
  // Header of datafile is number of detectors (i.e. Histogram1D's) in the workspace & detectors per block
  m_datafile.write((char *) &m_noVectors, sizeof(unsigned int));
  m_datafile.write((char *) &m_vectorsPerBlock, sizeof(unsigned int));
  // Fill main body of file with zeroes
  const std::vector<double> xzeroes(m_XLength, 0.0);
  const std::vector<double> yzeroes(m_YLength, 0.0);
  for (unsigned int i = 0; i < m_vectorsPerBlock*numberOfBlocks; ++i) 
  {
    m_datafile.write((char *) &*xzeroes.begin(), m_XLength * sizeof(double));
    m_datafile.write((char *) &*yzeroes.begin(), m_YLength * sizeof(double));
    m_datafile.write((char *) &*yzeroes.begin(), m_YLength * sizeof(double));
    m_datafile.write((char *) &*yzeroes.begin(), m_YLength * sizeof(double));
  }
  // Check we didn't run out of diskspace or something
  if ( m_datafile.fail() )
  {
    g_log.error("Unable to write temporary file");
    // probably throw
  }
  // Wondering if I could have file grow only if necessary, or if this would actually be slower
}

/// Destructor. Clears the buffer and deletes the temporary file.
ManagedWorkspace2D::~ManagedWorkspace2D()
{
//  std::cout << "Destructor called for " << getTitle() << std::endl;
  // delete all ManagedDataBlock2D's
  m_bufferedData.clear();
  // delete the temporary file
  m_datafile.close();
  remove(m_filename.c_str());
}

/// Returns the number of histograms
const int ManagedWorkspace2D::getHistogramNumber() const
{
  return static_cast<const int>(m_noVectors);
}

/// Get pseudo size
int ManagedWorkspace2D::size() const 
{ 
  return static_cast<int>(m_noVectors) * blocksize(); 
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
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
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
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
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
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
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
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
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
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, Vec, VecErr);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param Vec The data to enter
 *  @param VecErr The corresponding errors
 *  @param VecErr2 The corresponding error second values
 */
void ManagedWorkspace2D::setData(const int histnumber, const std::vector<double>& Vec, 
                                 const std::vector<double>& VecErr, const std::vector<double>& VecErr2)
{
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, Vec, VecErr, VecErr2);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param PY The data to enter
 */
void ManagedWorkspace2D::setData(const int histnumber, const Histogram1D::RCtype& PY)
{
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
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
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param PY The data to enter
 *  @param PE The corresponding errors
 *  @param PE2 The corresponding error second values
 */
void ManagedWorkspace2D::setData(const int histnumber, const Histogram1D::RCtype& PY, 
        const Histogram1D::RCtype& PE,const Histogram1D::RCtype& PE2)
{
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE, PE2);
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
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE);
  return;
}

/** Set the data values
 *  @param histnumber Index of the histogram to be set
 *  @param PY The data to enter
 *  @param PE The corresponding errors
 *  @param PE2 The corresponding error second values
 */
void ManagedWorkspace2D::setData(const int histnumber, const Histogram1D::RCtype::ptr_type& PY, 
        const Histogram1D::RCtype::ptr_type& PE, const Histogram1D::RCtype::ptr_type& PE2)
{
  if ( histnumber<0 || histnumber>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE, PE2);
  return;
}

/** Get the x data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the x data
 */
std::vector<double>& ManagedWorkspace2D::dataX(const int index)
{
  if ( index<0 || index>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::dataX, histogram number out of range");

  return getDataBlock(index)->dataX(index);
}

/** Get the y data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the y data
 */
std::vector<double>& ManagedWorkspace2D::dataY(const int index)
{
  if ( index<0 || index>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::dataY, histogram number out of range");

  return getDataBlock(index)->dataY(index);
}

/** Get the error data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the error data
 */
std::vector<double>& ManagedWorkspace2D::dataE(const int index)
{
  if ( index<0 || index>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::dataE, histogram number out of range");

  return getDataBlock(index)->dataE(index);
}

/** Get the error (second value) data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the error (second value) data
 */
std::vector<double>& ManagedWorkspace2D::dataE2(const int index)
{
  if ( index<0 || index>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::dataE2, histogram number out of range");

  return getDataBlock(index)->dataE2(index);
}

/** Get the x data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the x data
 */
const std::vector<double>& ManagedWorkspace2D::dataX(const int index) const
{
  if ( index<0 || index>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::dataX, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataX(index);
}

/** Get the y data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the y data
 */
const std::vector<double>& ManagedWorkspace2D::dataY(const int index) const
{
  if ( index<0 || index>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::dataY, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataY(index);
}

/** Get the error data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the error data
 */
const std::vector<double>& ManagedWorkspace2D::dataE(const int index) const
{
  if ( index<0 || index>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::dataE, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataE(index);
}

/** Get the error (second value) data of a specified histogram
 *  @param index The number of the histogram
 *  @return A vector of doubles containing the error (second value) data
 */
const std::vector<double>& ManagedWorkspace2D::dataE2(const int index) const
{
  if ( index<0 || index>=static_cast<int>(m_noVectors) )
    throw std::range_error("ManagedWorkspace2D::dataE2, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataE2(index);
}

/** Get a pointer to the data block containing the data corresponding to a given index
 *  @param index The index to search for
 *  @return A pointer to the data block containing the index requested
 */
// not really a const method, but need to make it that so const data getters can call it
ManagedDataBlock2D* ManagedWorkspace2D::getDataBlock(const int index) const
{
  unsigned int startIndex = index - ( index%m_vectorsPerBlock );
  /// @TODO Work out how to make 'find' work (for now, iterate through the collection)
  for (mru_list::const_iterator it = m_bufferedData.begin(); it != m_bufferedData.end(); ++it)
  {
    if ((*it)->minIndex() == startIndex)
    {
      return *it;
    }
  }
  
  // If not found, need to load block into memory and mru list
  ManagedDataBlock2D *newBlock = new ManagedDataBlock2D(startIndex, m_vectorsPerBlock, m_XLength, m_YLength);
  int seekPoint = ( 2 * sizeof(unsigned int) ) + ( startIndex * (m_XLength + ( 3*m_YLength )) * sizeof(double) );
  m_datafile.seekg(seekPoint, std::ios::beg);
  m_datafile >> *newBlock;
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
      int seekPoint = ( 2 * sizeof(unsigned int) ) + toWrite->minIndex() * (outer.m_XLength + ( 3*outer.m_YLength )) * sizeof(double);
      outer.m_datafile.seekp(seekPoint, std::ios::beg);
      outer.m_datafile << *toWrite;
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
