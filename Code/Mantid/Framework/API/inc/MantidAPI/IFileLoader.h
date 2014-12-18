#ifndef MANTID_API_IFILELOADER_H_
#define MANTID_API_IFILELOADER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/NexusDescriptor.h"

namespace Mantid
{
  namespace API
  {

    /**
    Defines an interface to an algorithm that loads a file so that it can take part in
    the automatic selection procedure provided by the FileLoaderRegistry.

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    template<typename DescriptorType>
    class MANTID_API_DLL IFileLoader : public Algorithm
    {
    public:
      /// Virtual destructor (required by linker on some versions of OS X/Intel compiler)
      virtual ~IFileLoader() {}
      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(DescriptorType & descriptor) const = 0;
      /// Returns a value indicating whether or not loader wants to load multiple files into a single workspace
      virtual bool loadMutipleAsOne() { return false; }
    };

  } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_IFILELOADER_H_ */
