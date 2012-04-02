#ifndef ABSMANAGEDWORKSPACE2D_H 
#define ABSMANAGEDWORKSPACE2D_H 

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/MRUList.h"
#include "MantidDataObjects/ManagedDataBlock2D.h"

namespace Mantid
{
namespace DataObjects
{

  /** For use in the AbsManagedWorkspace2D MRU list */
  class ManagedDataBlockMRUMarker
  {
  public:
    /** Constructor
     * @param dataBlockIndex :: index of the data block in the list of data blocks of the ManagedWorkspace2D
     */
    ManagedDataBlockMRUMarker(size_t dataBlockIndex)
    : m_index(dataBlockIndex)
    {
    }

    /// Function returns a unique index, used for hashing for MRU list
    size_t hashIndexFunction() const
    {
      return m_index;
    }

    /// @return index of the data block in the list of data blocks of the ManagedWorkspace2D
    size_t getBlockIndex() const
    {
      return m_index;
    }

  private:
    size_t m_index; ///< unique index of a data block

  };



  /** AbsManagedWorkspace2D

  This is an abstract class for a managed workspace. 
  Implementations must override init(..) which sets m_vectorsPerBlock and
  readDataBlock(..) and writeDataBlock(..)

  Copyright &copy; 2008-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    friend class ManagedHistogram1D;

  protected:
    /// Most-Recently-Used list of markers of ManagedDataBlocks objects.
    typedef Mantid::Kernel::MRUList<ManagedDataBlockMRUMarker> mru_list;

  public:
    AbsManagedWorkspace2D();
    virtual ~AbsManagedWorkspace2D();

    virtual const std::string id() const {return "AbsManagedWorkspace2D";}

    /// Return the underlying ISpectrum ptr at the given workspace index.
    virtual Mantid::API::ISpectrum * getSpectrum(const size_t index);

    /// Return the underlying ISpectrum ptr (const version) at the given workspace index.
    virtual const Mantid::API::ISpectrum * getSpectrum(const size_t index) const;

    //section required for iteration
    virtual std::size_t size() const;
    virtual std::size_t blocksize() const;

    /// Returns the size of physical memory the workspace takes
    virtual size_t getMemorySize() const = 0;

    /// Managed workspaces are not really thread-safe (and parallel file access
    /// would be silly anyway)
    virtual bool threadSafe() const { return false; }

  protected:
    /// Vector of the data blocks contained. All blocks are in memory but their contents might be empty
    std::vector<ManagedDataBlock2D *> m_blocks;

    /// Initialize
    virtual void init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength);

    /// Init the blocks alone
    void initBlocks();

    /// Number of blocks in temporary storage
    std::size_t getNumberBlocks() const
    {
      return m_bufferedMarkers.size();
    }

    /// Get a data block for a workspace index
    ManagedDataBlock2D* getDataBlock(const std::size_t index) const;

    /// Get and read in a data block only if required by the MRU lsit
    void readDataBlockIfNeeded(const std::size_t index) const;

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

    /// Markers used only to track which data blocks to release
    mutable mru_list m_bufferedMarkers;

  private:
    // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
    /// Private copy constructor
    AbsManagedWorkspace2D(const AbsManagedWorkspace2D&);
    /// Private copy assignment operator
    AbsManagedWorkspace2D& operator=(const AbsManagedWorkspace2D&);

    virtual std::size_t getHistogramNumberHelper() const;

  public:

  };

} // namespace DataObjects
} // namespace Mantid

#endif /* ABSMANAGEDWORKSPACE2D_H */
