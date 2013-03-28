#ifndef MANTID_KERNEL_SAVEABLE_H_
#define MANTID_KERNEL_SAVEABLE_H_
    
#include "MantidKernel/ISaveable.h"
#include <vector>
#include <algorithm>

namespace Mantid
{
namespace Kernel
{

  /** An interface for objects that can be cached or saved to disk.
    This is implemented by MDBox and is used in the in-memory
    cache of file-backed MDEventWorkspaces.
    
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport Saveable : public ISaveable
  {
  public:
    Saveable();
    //Saveable(const size_t id);
    Saveable(const Saveable & other);
    virtual ~Saveable();


    /// @return true if it the data of the object is busy and so cannot be cleared; false if the data was released and can be cleared/written.
    bool isBusy() const
    {   return m_Busy;  }

    /** Returns the state of the parameter, which tells disk buffer to force writing data 
     * to disk despite the size of the object have not changed (so one have probably done something with object contents. */
    bool isDataChanged()const
    {return m_dataChanged;}

    /** function returns true if the object have ever been saved on HDD and knows it place there*/
    bool wasSaved()const // for speed it returns this boolean, but for relaibility this should be m_wasSaved&&(m_fileIndexStart!=max())
    {  return m_wasSaved;    }

    bool isLoaded()const
    { return m_isLoaded;}


    void setBusy(bool On);
    void setDataChanged();
    void clearDataChanged();
    void setFilePosition(uint64_t newPos,size_t newSize,bool wasSaved);
    void setLoaded(bool Yes);

  protected:
    //-------------- 
    /// a user needs to set this variable to true preventing from deleting data from buffer
    bool m_Busy;
    /** a user needs to set this variable to true to allow DiskBuffer saving the object to HDD 
        when it decides it suitable,  if the size of iSavable object in cache is unchanged from the previous 
        save/load operation */
    bool m_dataChanged;
    // this tracks the history of operations, occuring over the data. 
    /// this boolean indicates if the data were saved on HDD and have physical representation on it (though this representation may be incorrect as data changed in memory)
    mutable bool m_wasSaved;
    /// this boolean indicates, if the data have its copy in memory 
    bool m_isLoaded;

    /// the function saveAt has to be availible to DiskBuffer and nobody else. To highlight this we make it private
    friend class DiskBuffer;

  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_ISAVEABLE_H_ */
