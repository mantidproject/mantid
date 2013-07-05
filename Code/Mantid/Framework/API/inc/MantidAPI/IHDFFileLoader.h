#ifndef MANTID_API_IHDFFILELOADER_H_
#define MANTID_API_IHDFFILELOADER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/HDFDescriptor.h"

namespace Mantid
{
  namespace API
  {
    /**

    Defines an interface to an algorithm that loads a file stored in a hierarchical data
    format. This supports anything the HDFDescriptor can wrap, currently = NeXus, HDF5

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport IHDFFileLoader : public API::Algorithm
    {
    public:
      /// Virtual destructor (required by linker on some versions of OS X/Intel compiler)
      virtual ~IHDFFileLoader() {}
      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::HDFDescriptor & descriptor) const = 0;
    };

  } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_IHDFFILELOADER_H_ */
