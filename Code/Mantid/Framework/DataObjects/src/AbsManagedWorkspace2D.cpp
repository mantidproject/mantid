//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/AbsManagedWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/ManagedDataBlock2D.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/ConfigService.h"

#include <fstream>
#include <valarray>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

using Mantid::API::ISpectrum;

namespace Mantid
{
namespace DataObjects
{

using std::size_t;


/// Constructor
AbsManagedWorkspace2D::AbsManagedWorkspace2D() :
Workspace2D()
{
}


//------------------------------------------------------------------------------
/** Sets the size of the workspace and sets up the temporary file.
 * The m_vectorsPerBlock value needs to be set by now.
 *
*  @param NVectors :: The number of vectors/histograms/detectors in the workspace
*  @param XLength :: The number of X data points/bin boundaries in each vector (must all be the same)
*  @param YLength :: The number of data/error points in each vector (must all be the same)
*  @throw std::runtime_error if unable to open a temporary file
*/
void AbsManagedWorkspace2D::init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength)
{
  m_noVectors = NVectors;
  m_axes.resize(2);
  m_axes[0] = new API::RefAxis(XLength, this);
  m_axes[1] = new API::SpectraAxis(this);
  m_XLength = XLength;
  m_YLength = YLength;
}

//------------------------------------------------------------------------------
/** Create all the blocks and spectra in the workspace
  * The m_vectorsPerBlock value needs to be set by now.
  * Must be called AFTER init()
  *
  */
void AbsManagedWorkspace2D::initBlocks()
{
  // Make a default-0 DX vector (X error vector). It will be shared by all spectra
  MantidVecPtr sharedDx;
  sharedDx.access().resize(m_XLength, 0.0);

  // Create all the data bloocks
  m_blocks.clear();
  for (size_t i=0; i<m_noVectors; i += m_vectorsPerBlock)
  {
    // Ensure the last block has the right # of vectors
    size_t numVectorsInThisBlock = m_vectorsPerBlock;
    if ((i + numVectorsInThisBlock) > m_noVectors) numVectorsInThisBlock = m_noVectors - i;
    // Each ManagedDataBlock2D will create its own vectors.
    ManagedDataBlock2D * block = new ManagedDataBlock2D(i, numVectorsInThisBlock, m_XLength, m_YLength, this, sharedDx );
    m_blocks.push_back( block );
  }

  // Copy the pointers over into the Workspace2D thing
  data.resize(m_noVectors, NULL);
  for (size_t i=0; i<m_noVectors; i++)
    data[i] = this->getSpectrum(i);
}



/// Destructor. Clears the buffer and deletes the temporary file.
AbsManagedWorkspace2D::~AbsManagedWorkspace2D()
{
  // Clear MRU list (minor amount of memory)
  m_bufferedMarkers.clear();
  // Delete the blocks (big memory);
  for (size_t i=0; i<m_blocks.size(); i++)
    delete m_blocks[i];
  m_blocks.clear();
}

/// Get pseudo size
size_t AbsManagedWorkspace2D::size() const
{
  return m_noVectors * blocksize();
}

/// Get the size of each vector
size_t AbsManagedWorkspace2D::blocksize() const
{
  return (m_noVectors > 0) ? m_YLength : 0;
}


//--------------------------------------------------------------------------------------------
/// Return the underlying ISpectrum ptr at the given workspace index.
ISpectrum * AbsManagedWorkspace2D::getSpectrum(const size_t index)
{
  if (index>=m_noVectors)
    throw std::range_error("AbsManagedWorkspace2D::getSpectrum, histogram number out of range");
  ISpectrum * spec = getDataBlock(index)->getSpectrum(index);
  return spec;
}


const ISpectrum * AbsManagedWorkspace2D::getSpectrum(const size_t index) const
{
  if (index>=m_noVectors)
    throw std::range_error("AbsManagedWorkspace2D::getSpectrum, histogram number out of range");

  ISpectrum * spec = const_cast<ISpectrum*>(getDataBlock(index)->getSpectrum(index));
  return spec;
}

//--------------------------------------------------------------------------------------------
/** Returns the number of histograms.
 *  For some reason Visual Studio couldn't deal with the main getHistogramNumber() method
 *  being virtual so it now just calls this private (and virtual) method which does the work.
 *  @return the number of histograms associated with the workspace
 */
size_t AbsManagedWorkspace2D::getHistogramNumberHelper() const
{
  return m_noVectors;
}


//--------------------------------------------------------------------------------------------
/** Get a pointer to the data block containing the data corresponding to a given index
*  @param index :: The index to search for
*  @return A pointer to the data block containing the index requested
*/
// not really a const method, but need to pretend it is so that const data getters can call it
ManagedDataBlock2D* AbsManagedWorkspace2D::getDataBlock(const std::size_t index) const
{
  // Which clock does this correspond to?
  size_t blockIndex = index / m_vectorsPerBlock;
  // Address of that block
  return const_cast<ManagedDataBlock2D*>(m_blocks[blockIndex]);
}


/** Get and read in a data block only if required by the MRU lsit
 *
 * @param index :: workspace index of the spectrum wanted
 */
void AbsManagedWorkspace2D::readDataBlockIfNeeded(const std::size_t index) const
{
  // Which block does this correspond to?
  size_t blockIndex = index / m_vectorsPerBlock;

  // Read it in first.
  ManagedDataBlock2D* readBlock = const_cast<ManagedDataBlock2D*>(m_blocks[blockIndex]);
  this->readDataBlock(readBlock, readBlock->minIndex());

  // Mark that this latest-read block was recently read into the MRU list
  ManagedDataBlockMRUMarker * markerToDrop = m_bufferedMarkers.insert(
      new ManagedDataBlockMRUMarker(blockIndex));

  // Do we need to drop out a data block?
  if (markerToDrop)
  {
    size_t dropIndex = markerToDrop->getBlockIndex();
    //std::cout << "Dropping block at " << dropIndex << " when asked to read block " << blockIndex << std::endl;
    delete markerToDrop;
    if (dropIndex < m_blocks.size())
    {
      // Address of that block
      ManagedDataBlock2D* droppedBlock = const_cast<ManagedDataBlock2D*>(m_blocks[dropIndex]);
      // Write it to disk only when needed
      if (droppedBlock->hasChanges())
        this->writeDataBlock(droppedBlock);
      // Free up the memory
      droppedBlock->releaseData();
    }
  }


}


} // namespace DataObjects
} // namespace Mantid
