#ifndef MANTID_KERNEL_WRITEBUFFER_H_
#define MANTID_KERNEL_WRITEBUFFER_H_

#include "MantidKernel/System.h"


namespace Mantid
{
namespace Kernel
{

  /** Replacement of the DiskMRU class that holds
    a "to-write" buffer and a list of free space blocks
    in the file to write.

    For use in file-backed MDEventWorkspaces.
    
    @date 2011-11-17

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
  class DLLExport WriteBuffer 
  {
  public:
    WriteBuffer();
    virtual ~WriteBuffer();
    
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_WRITEBUFFER_H_ */
