//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedDataBlock2D.h"
#include "MantidDataObjects/ManagedHistogram1D.h"
#include "MantidKernel/Exception.h"
#include <iostream>

using Mantid::API::ISpectrum;

namespace Mantid
{
namespace DataObjects
{

using std::size_t;

// Get a reference to the logger
Kernel::Logger& ManagedDataBlock2D::g_log = Kernel::Logger::get("ManagedDataBlock2D");

/** Constructor.
 *  @param minIndex :: The index of the workspace that this data block starts at
 *  @param NVectors :: The number of Histogram1D's in this data block
 *  @param XLength ::  The number of elements in the X data
 *  @param YLength ::  The number of elements in the Y/E data
 *  @param parentWS :: The workspace that owns this block
 *  @param sharedDx :: A cow-ptr to a correctly-sized DX vector that every spectrum will share (until written to).
 */
ManagedDataBlock2D::ManagedDataBlock2D(const std::size_t &minIndex, const std::size_t &NVectors,
    const std::size_t &XLength, const std::size_t &YLength, AbsManagedWorkspace2D * parentWS, MantidVecPtr sharedDx) :
  m_XLength(XLength), m_YLength(YLength), m_minIndex(minIndex), m_loaded(false)
{
//  MantidVecPtr t1;
//  t1.access().resize(XLength); //this call initializes array to zero

  m_data.clear();
  for (size_t i=0; i<NVectors; i++)
  {
    m_data.push_back( new ManagedHistogram1D(parentWS, minIndex+i) );
    m_data[i]->setDx(sharedDx);
    //TODO: Anything about Dx?
  }
  if (!parentWS)
    this->initialize();
}

//-----------------------------------------------------------------------
/** Initialize the vectors to empty values */
void ManagedDataBlock2D::initialize()
{
  for (size_t i=0; i<m_data.size(); i++)
  {
    // Initialize the vectors if no workspace is passed
    m_data[i]->directDataX().resize(m_XLength, 0.0);
    m_data[i]->directDataY().resize(m_YLength, 0.0);
    m_data[i]->directDataE().resize(m_YLength, 0.0);
    // It was 'loaded' so it won't get re-initialized in this block
    m_data[i]->setLoaded(true);
  }
  this->m_loaded = true;
}

/// Virtual destructor
ManagedDataBlock2D::~ManagedDataBlock2D()
{
  // Do nothing: Workspace2D owns the pointer!!!
}

/// The minimum index of the workspace data that this data block contains
int ManagedDataBlock2D::minIndex() const // TODO this should return size_t but it breaks lots of things
{
  return static_cast<int>(m_minIndex);
}

/** Flags whether the data has changed since being read in.
 *  In fact indicates whether the data has been accessed in a non-const fashion.
 *  @return True if the data has been changed.
 */
bool ManagedDataBlock2D::hasChanges() const
{
  bool hasChanges = false;
  for (std::vector<ManagedHistogram1D*>::const_iterator iter = m_data.begin(); iter != m_data.end(); ++iter)
  {
    const ManagedHistogram1D * it = *iter;
    hasChanges = hasChanges || it->isDirty();
  }
  return hasChanges;
}

/** Gives the possibility to drop the flag. Used in ManagedRawFileWorkspace2D atfer
 *  reading in from a raw file.
 *  @param has :: True if the data has been changed.
 */
void ManagedDataBlock2D::hasChanges(bool has)
{
  for (std::vector<ManagedHistogram1D*>::iterator iter = m_data.begin(); iter != m_data.end(); ++iter)
  {
    ManagedHistogram1D * it = *iter;
    it->m_dirty = has;
  }
}



//--------------------------------------------------------------------------------------------
/// Return the underlying ISpectrum ptr at the given workspace index.
ISpectrum * ManagedDataBlock2D::getSpectrum(const size_t index)
{
  if ( ( index < m_minIndex )
      || ( index >= m_minIndex + m_data.size() ) )
    throw std::range_error("ManagedDataBlock2D::getSpectrum, histogram number out of range");
  return m_data[index-m_minIndex];
}

const ISpectrum * ManagedDataBlock2D::getSpectrum(const size_t index) const
{
  if ( ( index < m_minIndex )
      || ( index >= m_minIndex + m_data.size() ) )
    throw std::range_error("ManagedDataBlock2D::getSpectrum, histogram number out of range");
  return m_data[index-m_minIndex];
}


//--------------------------------------------------------------------------------------------
/** Release the memory in the loaded data blocks */
void ManagedDataBlock2D::releaseData()
{
  for (std::vector<ManagedHistogram1D*>::iterator iter = m_data.begin(); iter != m_data.end(); ++iter)
  {
    ManagedHistogram1D * it = *iter;
    it->releaseData();
  }
  this->m_loaded = false;
}



/** Output file stream operator.
 *  @param fs :: The stream to write to
 *  @param data :: The object to write to file
 *  @return stream representation of data
 */
std::fstream& operator<<(std::fstream& fs, ManagedDataBlock2D& data)
{
  for (std::vector<ManagedHistogram1D*>::iterator iter = data.m_data.begin(); iter != data.m_data.end(); ++iter)
  {
    ManagedHistogram1D * it = *iter;
    // If anyone's gone and changed the size of the vectors then get them back to the
    // correct size, removing elements or adding zeroes as appropriate.
    if (it->directDataX().size() != data.m_XLength)
    {
      it->directDataX().resize(data.m_XLength, 0.0);
      ManagedDataBlock2D::g_log.warning() << "X vector resized to " << data.m_XLength << " elements.";
    }
    fs.write(reinterpret_cast<char *>(&(it->directDataX().front())), data.m_XLength * sizeof(double));
    if (it->directDataY().size() != data.m_YLength)
    {
      it->directDataY().resize(data.m_YLength, 0.0);
      ManagedDataBlock2D::g_log.warning() << "Y vector resized to " << data.m_YLength << " elements.";
    }
    fs.write(reinterpret_cast<char *>(&(it->directDataY().front())), data.m_YLength * sizeof(double));
    if (it->directDataE().size() != data.m_YLength)
    {
      it->directDataE().resize(data.m_YLength, 0.0);
      ManagedDataBlock2D::g_log.warning() << "E vector resized to " << data.m_YLength << " elements.";
    }
    fs.write(reinterpret_cast<char *>(&(it->directDataE().front())), data.m_YLength * sizeof(double));
    
    // Clear the "dirty" flag since it was just written out.
    it->setDirty(false);
  }
  return fs;
}

/** Input file stream operator.
 *  @param fs :: The stream to read from
 *  @param data :: The object to fill with the read-in data
 *  @return stream representation of data
 */
std::fstream& operator>>(std::fstream& fs, ManagedDataBlock2D& data)
{ 
  // Assumes that the ManagedDataBlock2D passed in is the same size as the data to be read in
  // Will be ManagedWorkspace2D's job to ensure that this is so
  for (std::vector<ManagedHistogram1D*>::iterator iter = data.m_data.begin(); iter != data.m_data.end(); ++iter)
  {
    ManagedHistogram1D * it = *iter;
    it->directDataX().resize(data.m_XLength, 0.0);
    fs.read(reinterpret_cast<char *>(&(it->directDataX().front())), data.m_XLength * sizeof(double));
    it->directDataY().resize(data.m_YLength, 0.0);
    fs.read(reinterpret_cast<char *>(&(it->directDataY().front())), data.m_YLength * sizeof(double));
    it->directDataE().resize(data.m_YLength, 0.0);
    fs.read(reinterpret_cast<char *>(&(it->directDataE().front())), data.m_YLength * sizeof(double));
    // Yes, it is loaded
    it->setLoaded(true);
  }
  return fs;
}

} // namespace DataObjects
} // namespace Mantid
