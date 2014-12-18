#ifndef MANTID_API_ARCHIVESEARCHFACTORY_H_
#define MANTID_API_ARCHIVESEARCHFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
  namespace API
  {
    class IArchiveSearch;

    /**
    Creates instances of IArchiveSearch

    @author Roman Tolchenov, Tessella plc
    @date 27/07/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTID_API_DLL ArchiveSearchFactoryImpl : public Kernel::DynamicFactory<IArchiveSearch>
    {
    private:
      friend struct Mantid::Kernel::CreateUsingNew<ArchiveSearchFactoryImpl>;

      /// Private Constructor for singleton class
      ArchiveSearchFactoryImpl();
      /// Private copy constructor - NO COPY ALLOWED
      ArchiveSearchFactoryImpl(const ArchiveSearchFactoryImpl&);
      /// Private assignment operator - NO ASSIGNMENT ALLOWED
      ArchiveSearchFactoryImpl& operator = (const ArchiveSearchFactoryImpl&);
      ///Private Destructor
      virtual ~ArchiveSearchFactoryImpl(){}
    };

///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<ArchiveSearchFactoryImpl>;
#endif /* _WIN32 */

typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<ArchiveSearchFactoryImpl> ArchiveSearchFactory;


  }
}

#endif //MANTID_API_ARCHIVESEARCHFACTORY_H_
