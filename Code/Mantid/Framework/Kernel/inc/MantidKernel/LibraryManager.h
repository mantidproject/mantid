#ifndef MANTID_KERNEL_LIBRARY_MANAGER_H_
#define MANTID_KERNEL_LIBRARY_MANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include <map>
#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif

#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/DllConfig.h"

namespace Mantid
{
  namespace Kernel
  {
    class LibraryWrapper;

    /** 
    Class for opening shared libraries.

    @author ISIS, STFC
    @date 15/10/2007

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class MANTID_KERNEL_DLL LibraryManagerImpl
    {
    public:
      //opens all suitable libraries on a given path
      int OpenAllLibraries(const std::string&, bool isRecursive=false);
    private:
      friend struct Mantid::Kernel::CreateUsingNew<LibraryManagerImpl>;

      ///Private Constructor
      LibraryManagerImpl();
      /// Private copy constructor - NO COPY ALLOWED
      LibraryManagerImpl(const LibraryManagerImpl&);
      /// Private assignment operator - NO ASSIGNMENT ALLOWED
      LibraryManagerImpl& operator = (const LibraryManagerImpl&);
      ///Private Destructor
      virtual ~LibraryManagerImpl();

      /// Load a given library
      bool loadLibrary(const std::string & filepath);
      /// Returns true if the library is to be loaded
      bool skip(const std::string & filename);
      ///Storage for the LibraryWrappers.
      std::map< const std::string, boost::shared_ptr<Mantid::Kernel::LibraryWrapper> > OpenLibs;
    };

    ///Forward declaration of a specialisation of SingletonHolder for LibraryManagerImpl (needed for dllexport/dllimport) and a typedef for it.
#if defined(__APPLE__) && defined(__INTEL_COMPILER)
    inline
#endif
      template class MANTID_KERNEL_DLL Mantid::Kernel::SingletonHolder<LibraryManagerImpl>;
    typedef MANTID_KERNEL_DLL Mantid::Kernel::SingletonHolder<LibraryManagerImpl> LibraryManager;

  } // namespace Kernel
} // namespace Mantid

#endif //MANTID_KERNEL_LIBRARY_MANAGER_H_
