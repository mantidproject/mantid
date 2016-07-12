#ifndef WINDOWFACTORY_H
#define WINDOWFACTORY_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class MdiSubWindow;

namespace Mantid {
namespace API {

/** The WindowFactory class is in charge of the creation of concrete
    instances of MdiSubWindow. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.

    @author Samuel Jackson, ISIS Rutherford Appleton Laboratory
    @date 11/07/2016

    Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
*/

class MANTID_API_DLL WindowFactoryImpl final
    : public Kernel::DynamicFactory<MdiSubWindow> {
public:
  WindowFactoryImpl();
};

/// Forward declaration of a specialisation of SingletonHolder for
/// AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<WindowFactoryImpl>;
#endif /* _WIN32 */

typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<WindowFactoryImpl>
    WindowFactory;
}
}

#endif // WINDOWFACTORY_H
