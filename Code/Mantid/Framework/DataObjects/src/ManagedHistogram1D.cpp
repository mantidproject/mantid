#include "MantidDataObjects/ManagedHistogram1D.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/AbsManagedWorkspace2D.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"

namespace Mantid
{
namespace DataObjects
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ManagedHistogram1D::ManagedHistogram1D(AbsManagedWorkspace2D * parentWS, size_t workspaceIndex)
  : m_loaded(false), m_dirty(false),
    m_parentWorkspace(parentWS), m_workspaceIndex(workspaceIndex)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ManagedHistogram1D::~ManagedHistogram1D()
  {
  }
  

  //------------------------------------------------------------------------
  /** Clear Y and E vectors */
  void ManagedHistogram1D::clearData()
  {
    retrieveData();
    m_dirty = true;
    MantidVec & yValues = this->dataY();
    std::fill(yValues.begin(), yValues.end(), 0.0);
    MantidVec & eValues = this->dataE();
    std::fill(eValues.begin(), eValues.end(), 0.0);
  }


  //------------------------------------------------------------------------
  /** Method that retrieves the data from the disk
   * if needed.
   */
  void ManagedHistogram1D::retrieveData() const
  {
    // Only load from disk when needed
    if (!m_loaded)
    {
      if (m_parentWorkspace)
      {
        // This method will read in the data and fill in this (and other nearby) spectra
        m_parentWorkspace->readDataBlockIfNeeded(m_workspaceIndex);
      }
      // OK, we're loaded
      m_loaded = true;
      // We have not modified anything yet.
      m_dirty = false;
    }
  }


  //------------------------------------------------------------------------
  /** Method that clears the data vectors to release
   * memory when the spectrum can be released to disk.
   * The data should have been saved already.
   */
  void ManagedHistogram1D::releaseData()
  {
    if (m_loaded)
    {
      // Clear all the vectors to release the memory
      refX.access().clear();
      MantidVec().swap(refX.access());
      refY.access().clear();
      MantidVec().swap(refY.access());
      refE.access().clear();
      MantidVec().swap(refE.access());
      // Note: we leave DX alone since it is kept in memory always.
      // Reset Markers
      m_loaded = false;
      m_dirty = false;
    }
  }

} // namespace Mantid
} // namespace DataObjects

