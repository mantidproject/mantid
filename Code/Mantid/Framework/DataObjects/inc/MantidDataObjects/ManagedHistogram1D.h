#ifndef MANTID_DATAOBJECTS_MANAGEDHISTOGRAM1D_H_
#define MANTID_DATAOBJECTS_MANAGEDHISTOGRAM1D_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/Histogram1D.h"
//#include "MantidDataObjects/AbsManagedWorkspace2D.h"
//#include "MantidDataObjects/ManagedDataBlock2D.h"


namespace Mantid
{
namespace DataObjects
{

  // Forward declaration
  class AbsManagedWorkspace2D;

  /** A "managed" version of Histogram1D where the
   * data is loaded only when required
    
    @author Janik Zikovsky
    @date 2011-07-26

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport ManagedHistogram1D : public Histogram1D //Mantid::API::ISpectrum
  {
  public:
    friend class ManagedDataBlock2D;

    ManagedHistogram1D(AbsManagedWorkspace2D * parentWS, size_t workspaceIndex);

    ManagedHistogram1D(const ManagedHistogram1D&);
    ManagedHistogram1D& operator=(const ManagedHistogram1D&);
    virtual ~ManagedHistogram1D();

    //------------------------------------------------------------------------
    /** Method that retrieves the data from the disk
     * if needed.
     */
    void retrieveData() const;

    /** Method that clears the data vectors to release
     * memory when the spectrum can be released to disk.
     */
    void releaseData();

    // ------------ Y/E data setters -----------------------------
    /// Sets the data.
    void setData(const MantidVec& Y)
    {  retrieveData(); refY.access()=Y; m_dirty=true; }
    /// Sets the data and errors
    void setData(const MantidVec& Y, const MantidVec& E)
    {  retrieveData(); refY.access()=Y; refE.access()=E; m_dirty=true; }

    /// Sets the data.
    void setData(const MantidVecPtr& Y)
    { retrieveData(); refY=Y; m_dirty=true; }
    /// Sets the data and errors
    void setData(const MantidVecPtr& Y, const MantidVecPtr& E)
    { retrieveData(); refY=Y; refE=E; m_dirty=true; }

    /// Sets the data.
    void setData(const MantidVecPtr::ptr_type& Y)
    { retrieveData(); refY=Y; m_dirty=true; }
    /// Sets the data and errors
    void setData(const MantidVecPtr::ptr_type& Y, const MantidVecPtr::ptr_type& E)
    { retrieveData(); refY=Y; refE=E; m_dirty=true; }

    /// Zero the data (Y&E) in this spectrum
    void clearData();

    // ------------ Y/E data accessors -----------------------------
    // Get the array data
    /// Returns the y data const
    virtual const MantidVec& dataY() const
    { retrieveData(); return *refY; }
    /// Returns the error data const
    virtual const MantidVec& dataE() const
    { retrieveData(); return *refE; }

    ///Returns the y data
    virtual MantidVec& dataY()
    { retrieveData(); m_dirty=true; return refY.access(); }
    ///Returns the error data
    virtual MantidVec& dataE()
    { retrieveData(); m_dirty=true; return refE.access(); }

    /// Returns the y data const
    virtual const MantidVec& readY() const
    { retrieveData(); return *refY; }
    /// Returns the error data const
    virtual const MantidVec& readE() const
    { retrieveData(); return *refE; }

    // ------------ X data accessors -----------------------------
    virtual void setX(const MantidVec& X)
    { retrieveData(); refX.access()=X; m_dirty = true; }

    virtual void setX(const MantidVecPtr& X)
    { retrieveData(); refX=X; m_dirty = true; }

    virtual void setX(const MantidVecPtr::ptr_type& X)
    { retrieveData(); refX=X; m_dirty = true; }

    virtual MantidVec& dataX()
    { retrieveData(); m_dirty = true;
      return refX.access(); }

    virtual const MantidVec& dataX() const
    { retrieveData();
      return *refX; }

    virtual const MantidVec& readX() const
    { retrieveData();
      return *refX; }

    virtual MantidVecPtr ptrX() const
    { retrieveData(); m_dirty = true;
      return refX; }

    virtual std::size_t size() const
    { retrieveData();
      return refY->size();
    }

    /// Checks for errors
    bool isError() const
    { retrieveData();
      return refE->empty();
    }

    /// Gets the memory size of the histogram
    size_t getMemorySize() const
    { return ((refX->size()+refY->size()+refE->size())*sizeof(double)); }

    //------------------------------------------------------------------------
    /// @return true if the data was modified.
    bool isDirty() const
    { return m_dirty; }

    /** Set the dirty flag
     * @param dirty :: bool flag value */
    void setDirty(bool dirty)
    { m_dirty = dirty; }

    //------------------------------------------------------------------------
    /// @return true if the data was loaded from disk
    bool isLoaded() const
    { return m_loaded; }

    /** Set the loaded flag
     * @param loaded :: bool flag value */
    void setLoaded(bool loaded)
    { m_loaded = loaded; }

    //------------------------------------------------------------------------
    /// @return m_workspaceIndex (for debugging mostly)
    size_t getWorkspaceIndex() const
    { return m_workspaceIndex; }

  protected:
    /// Are the data vectors loaded from the disk?
    mutable bool m_loaded;

    /// Was the data modified?
    mutable bool m_dirty;

    /// Workspace that owns this histogram
    AbsManagedWorkspace2D * m_parentWorkspace;

    /// Index in m_parentWorkspace
    size_t m_workspaceIndex;

  public:
    // ------------ Direct data accessors, for use by ManagedDataBlock2D -----------------------------
    MantidVec& directDataX() { return refX.access(); }
    MantidVec& directDataY() { return refY.access(); }
    MantidVec& directDataE() { return refE.access(); }
    void directSetX(boost::shared_ptr<MantidVec> newX) { refX = newX; }

  };


} // namespace DataObjects
} // namespace Mantid

#endif  /* MANTID_DATAOBJECTS_MANAGEDHISTOGRAM1D_H_ */
