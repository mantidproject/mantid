#ifndef COMPRESSEDWORKSPACE2D_H
#define COMPRESSEDWORKSPACE2D_H

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/AbsManagedWorkspace2D.h"

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
  /** CompressedWorkspace2D.

  Works similar to the ManagedWorkspace2D but keeps the data compressed in memory instead of the disk.
  Set CompressedWorkspace.VectorsPerBlock in .properties file to change the number of spectra in each of the
  100 blocks that are kept uncompressed for quick access.
  Set CompressedWorkspace.DoNotUse to switch off the use of the CompressedWorkspace2D.

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
  class DLLExport CompressedWorkspace2D : public AbsManagedWorkspace2D
  {
  public:

    /// Constructor
    CompressedWorkspace2D();
    /// Destructor
    ~CompressedWorkspace2D();
    virtual const std::string id() const {return "CompressedWorkspace2D";}
    /// Returns the size of physical memory the workspace takes
    size_t getMemorySize() const;

  protected:

    virtual void init(const int &NVectors, const int &XLength, const int &YLength);

    /// Reads in a data block.
    virtual void readDataBlock(ManagedDataBlock2D *newBlock,int startIndex)const;
    /// Saves the dropped data block to disk.
    virtual void writeDataBlock(ManagedDataBlock2D *toWrite) const;

  private:
    // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
    /// Private copy constructor
    CompressedWorkspace2D(const CompressedWorkspace2D&);
    /// Private copy assignment operator
    CompressedWorkspace2D& operator=(const CompressedWorkspace2D&);

    /// Type of a pointer to a compressed data block along with its size
    typedef std::pair<unsigned char*,size_t> CompressedPointer;
    /// Map of the compressed data storage
    typedef std::map<size_t,CompressedPointer > CompressedMap;

    /// Compresses a block
    CompressedPointer compressBlock(ManagedDataBlock2D* block,int startIndex) const;
    /// Uncompress a block
    void uncompressBlock(ManagedDataBlock2D* block,int startIndex)const;

    /// Data buffer used in compression and decompression
    mutable MantidVec m_inBuffer;
    /// Data buffer used in compression and decompression
    mutable std::vector<unsigned char> m_outBuffer;

    /// Keeps all compressed data
    mutable CompressedMap m_compressedData;

    /// Static reference to the logger class
    static Kernel::Logger &g_log;
  };



} // namespace DataObjects
} // namespace Mantid

#endif /* COMPRESSEDWORKSPACE2D_H */
