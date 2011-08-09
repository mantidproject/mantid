#ifndef MANTID_KERNEL_IFILE_H_
#define MANTID_KERNEL_IFILE_H_
    
#include "MantidKernel/System.h"


namespace Mantid
{
namespace Kernel
{

  /** Abstract interface to be used by DiskMRU
   * for saving to a file that can be extended in size.
    
    @author Janik Zikovsky
    @date 2011-08-08

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
  class DLLExport IFile 
  {
  public:
    IFile();
    virtual ~IFile();

    /// Return the length of the file, in units compatible with DiskMRU (not necessarily bytes).
    virtual uint64_t getFileLength() const = 0;

    /// Increase the size of the file to the given length.
    virtual void extendFile(uint64_t newLength) = 0;
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_IFILE_H_ */
