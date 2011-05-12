#ifndef ABSMANAGEDWORKSPACE2D_H 
#define ABSMANAGEDWORKSPACE2D_H 

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/MRUList.h"
#include "MantidDataObjects/ManagedDataBlock2D.h"

namespace Mantid
{
namespace DataObjects
{
  /** AbsManagedWorkspace2D

  This is an abstract class for a managed workspace. 
  Implementations must override init(..) which sets m_vectorsPerBlock and
  readDataBlock(..) and writeDataBlock(..)

  @author Roman Tolchenov, Tessella plc
  @date 28/07/2009

  Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport AbsManagedWorkspace2D : public Workspace2D
  {
  protected:
    /// Most-Recently-Used list of ManagedDataBlock2D objects.
    typedef Mantid::Kernel::MRUList<ManagedDataBlock2D> mru_list;
    //friend class mru_list;

  public:
    AbsManagedWorkspace2D(std::size_t NBlocks=100);
    virtual ~AbsManagedWorkspace2D();

    virtual const std::string id() const {return "AbsManagedWorkspace2D";}

    virtual void setX(const std::size_t histnumber, const MantidVecPtr&);
    virtual void setX(const std::size_t histnumber, const MantidVecPtr::ptr_type&);
    virtual void setData(std::size_t const histnumber, const MantidVecPtr&);
    virtual void setData(std::size_t const histnumber, const MantidVecPtr&, const MantidVecPtr&);
    virtual void setData(std::size_t const histnumber, const MantidVecPtr::ptr_type&, const MantidVecPtr::ptr_type&);

    //section required for iteration
    virtual std::size_t size() const;
    virtual std::size_t blocksize() const;

    virtual MantidVec& dataX(const std::size_t index);
    virtual MantidVec& dataY(const std::size_t index);
    virtual MantidVec& dataE(const std::size_t index);
    virtual const MantidVec& dataX(std::size_t const index) const;
    virtual const MantidVec& dataY(std::size_t const index) const;
    virtual const MantidVec& dataE(std::size_t const index) const;
    virtual Kernel::cow_ptr<MantidVec> refX(const std::size_t index) const;

    /// Returns the size of physical memory the workspace takes
    virtual size_t getMemorySize() const = 0;
    virtual bool threadSafe() const { return false; }

  protected:

    virtual void init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength);
    /// Number of blocks in temporary storage
    std::size_t getNumberBlocks() const
    {return m_bufferedData.size();}

    /// Reads in a data block.
    virtual void readDataBlock(ManagedDataBlock2D *newBlock,size_t startIndex)const = 0;
    /// Saves the dropped data block to disk.
    virtual void writeDataBlock(ManagedDataBlock2D *toWrite) const = 0;

    /// The number of vectors in each data block
    std::size_t m_vectorsPerBlock;
    /// The length of the X vector in each Histogram1D. Must all be the same.
    std::size_t m_XLength;
    /// The length of the Y & E vectors in each Histogram1D. Must all be the same.
    std::size_t m_YLength;
    /// The size in bytes of each vector
    std::size_t m_vectorSize;
    /// The size in bytes of one block
    std::size_t m_blockSize;

    /// Static reference to the logger class
    static Kernel::Logger &g_log;

    /// The most-recently-used list of buffered data blocks
    mutable mru_list m_bufferedData;

  private:
    // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
    /// Private copy constructor
    AbsManagedWorkspace2D(const AbsManagedWorkspace2D&);
    /// Private copy assignment operator
    AbsManagedWorkspace2D& operator=(const AbsManagedWorkspace2D&);

    virtual std::size_t getHistogramNumberHelper() const;

    ManagedDataBlock2D* getDataBlock(const std::size_t index) const;


  public:
    /// Callback func
    template <class ManagedDataBlock2D>
    void dropItemCallback(ManagedDataBlock2D* item_to_write_maybe)
    {
      std::cout << "dropItemCallback called!" << std::endl;
    }

  };

} // namespace DataObjects
} // namespace Mantid

#endif /* ABSMANAGEDWORKSPACE2D_H */
