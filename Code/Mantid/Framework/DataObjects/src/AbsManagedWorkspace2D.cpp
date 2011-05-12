//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/AbsManagedWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/ManagedDataBlock2D.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/ConfigService.h"

#include <fstream>
#include <valarray>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Mantid
{
namespace DataObjects
{

using std::size_t;

// Get a reference to the logger
Kernel::Logger& AbsManagedWorkspace2D::g_log = Kernel::Logger::get("AbsManagedWorkspace2D");


/// Constructor
AbsManagedWorkspace2D::AbsManagedWorkspace2D(size_t NBlocks) :
Workspace2D(), m_bufferedData(NBlocks)
{
}


/** Sets the size of the workspace and sets up the temporary file
*  @param NVectors :: The number of vectors/histograms/detectors in the workspace
*  @param XLength :: The number of X data points/bin boundaries in each vector (must all be the same)
*  @param YLength :: The number of data/error points in each vector (must all be the same)
*  @throw std::runtime_error if unable to open a temporary file
*/
void AbsManagedWorkspace2D::init(const size_t &NVectors, const size_t &XLength, const size_t &YLength)
{
  m_noVectors = NVectors;
  m_axes.resize(2);
  m_axes[0] = new API::RefAxis(XLength, this);
  m_axes[1] = new API::SpectraAxis(NVectors);
  m_XLength = XLength;
  m_YLength = YLength;

  m_vectorSize = ( m_XLength + ( 2*m_YLength ) ) * sizeof(double);

  // Define m_vectorsPerBlock in the init() of the derived class

}

/// Destructor. Clears the buffer and deletes the temporary file.
AbsManagedWorkspace2D::~AbsManagedWorkspace2D()
{
  // delete all ManagedDataBlock2D's
  m_bufferedData.clear();
}

/// Get pseudo size
size_t AbsManagedWorkspace2D::size() const
{
  return m_noVectors * blocksize();
}

/// Get the size of each vector
size_t AbsManagedWorkspace2D::blocksize() const
{
  return (m_noVectors > 0) ? static_cast<int>(m_YLength) : 0;
}

/** Set the x values
*  @param histnumber :: Index of the histogram to be set
*  @param PA :: The data to enter
*/
void AbsManagedWorkspace2D::setX(const size_t histnumber, const MantidVecPtr& PA)
{
  if ( histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setX, histogram number out of range");

  getDataBlock(histnumber)->setX(histnumber, PA);
  return;
}

/** Set the x valuesMantid
*  @param histnumber :: Index of the histogram to be set
*  @param Vec :: The data to enter
*/
void AbsManagedWorkspace2D::setX(const size_t histnumber, const MantidVecPtr::ptr_type& Vec)
{
  if ( histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setX, histogram number out of range");

  getDataBlock(histnumber)->setX(histnumber, Vec);
  return;
}

/** Set the data values
*  @param histnumber :: Index of the histogram to be set
*  @param PY :: The data to enter
*/
void AbsManagedWorkspace2D::setData(const size_t histnumber, const MantidVecPtr& PY)
{
  if ( histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY);
  return;
}

/** Set the data values
*  @param histnumber :: Index of the histogram to be set
*  @param PY :: The data to enter
*  @param PE :: The corresponding errors
*/
void AbsManagedWorkspace2D::setData(const size_t histnumber, const MantidVecPtr& PY,
                                    const MantidVecPtr& PE)
{
  if ( histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE);
  return;
}

/** Set the data values
*  @param histnumber :: Index of the histogram to be set
*  @param PY :: The data to enter
*  @param PE :: The corresponding errors
*/
void AbsManagedWorkspace2D::setData(const size_t histnumber, const MantidVecPtr::ptr_type& PY,
                                    const MantidVecPtr::ptr_type& PE)
{
  if ( histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE);
  return;
}

/** Get the x data of a specified histogram
*  @param index :: The number of the histogram
*  @return A vector of doubles containing the x data
*/
MantidVec& AbsManagedWorkspace2D::dataX(const size_t index)
{
  if ( index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataX, histogram number out of range");

  return getDataBlock(index)->dataX(index);
}

/** Get the y data of a specified hMRUList<ManagedDataBlock2D>istogram
*  @param index :: The number of the histogram
*  @return A vector of doubles containing the y data
*/
MantidVec& AbsManagedWorkspace2D::dataY(const size_t index)
{
  if ( index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataY, histogram number out of range");

  return getDataBlock(index)->dataY(index);
}

/** Get the error data of a specified histogram
*  @param index :: The number of the histogram
*  @return A vector of doubles containing the error data
*/
MantidVec& AbsManagedWorkspace2D::dataE(const size_t index)
{
  if ( index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataE, histogram number out of range");

  return getDataBlock(index)->dataE(index);
}

/** Get the x data of a specified histogram
*  @param index :: The number of the histogram
*  @return A vector of doubles containing the x data
*/
const MantidVec& AbsManagedWorkspace2D::dataX(const size_t index) const
{
  if ( index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataX, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataX(index);
}

/** Get the y data of a specified histogram
*  @param index :: The number of the histogram
*  @return A vector of doubles containing the y data
*/
const MantidVec& AbsManagedWorkspace2D::dataY(const size_t index) const
{
  if ( index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataY, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataY(index);
}

/** Get the error data of a specified histogram
*  @param index :: The number of the histogram
*  @return A vector of doubles containing the error data
*/
const MantidVec& AbsManagedWorkspace2D::dataE(const size_t index) const
{
  if ( index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataE, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataE(index);
}

Kernel::cow_ptr<MantidVec> AbsManagedWorkspace2D::refX(const size_t index) const
{
  if ( index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataX, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->refX(index);
}

/** Returns the number of histograms.
 *  For some reason Visual Studio couldn't deal with the main getHistogramNumber() method
 *  being virtual so it now just calls this private (and virtual) method which does the work.
 *  @return the number of histograms associated with the workspace
 */
size_t AbsManagedWorkspace2D::getHistogramNumberHelper() const
{
  return m_noVectors;
}

/** Get a pointer to the data block containing the data corresponding to a given index
*  @param index :: The index to search for
*  @return A pointer to the data block containing the index requested
*/
// not really a const method, but need to pretend it is so that const data getters can call it
ManagedDataBlock2D* AbsManagedWorkspace2D::getDataBlock(const size_t index) const
{
  std::cout << "AbsManagedWorkspace2d::getDataBlock(" << index << ")" << std::endl; // REMOVE
  size_t startIndex = index - ( index%m_vectorsPerBlock );
  std::cout << "   startIndex = " << startIndex << std::endl; // REMOVE

  // Look to see if the data block is already buffered
  ManagedDataBlock2D *existingBlock =  m_bufferedData.find(startIndex);
  if (existingBlock)
    return existingBlock;

  // If not found, need to load block into memory and mru list
  ManagedDataBlock2D *newBlock = new ManagedDataBlock2D(startIndex, m_vectorsPerBlock, m_XLength, m_YLength);
  // Check whether datablock has previously been saved. If so, read it in.
  readDataBlock(newBlock,startIndex);

  //Put the read block in the MRU
  ManagedDataBlock2D *toWrite = m_bufferedData.insert(newBlock);
  if (toWrite)
  {
    //We got out a block - it is a block that is being dropped.
    //If it changed, we need to save it
    if (toWrite->hasChanges())
      this->writeDataBlock(toWrite);
    //And it is up to us to delete the block now.
    delete toWrite;
  }

  return newBlock;
}


} // namespace DataObjects
} // namespace Mantid
