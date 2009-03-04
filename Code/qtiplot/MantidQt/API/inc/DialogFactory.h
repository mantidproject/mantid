#ifndef MANTIDQT_API_DIALOGFACTORY_H_
#define MANTIDQT_API_DIALOGFACTORY_H_

//------------------------
// Includes
//------------------------
#include "DllOption.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace MantidQt
{

namespace API
{

//------------------------
// Forward declaration
//------------------------
class AlgorithmDialog;

/** 
    The DialogFactory is responsible for creating concrete instances of
    AlgorithmDialog classes. It is implemented as a singleton class.
    
    @author Martyn Gigg, Tessella plc
    @date 24/02/2009
    
    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

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
*/
class EXPORT_OPT_MANTIDQT_API DialogFactoryImpl : public Mantid::Kernel::DynamicFactory<AlgorithmDialog>
{

public:
  // Unhide the inherited create method
  using Mantid::Kernel::DynamicFactory<AlgorithmDialog>::createUnwrapped;

private:
  friend struct Mantid::Kernel::CreateUsingNew<DialogFactoryImpl>;

  /// Private Constructor for singleton class
  DialogFactoryImpl();
  /// Private copy constructor - NO COPY ALLOWED
  DialogFactoryImpl(const DialogFactoryImpl&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  DialogFactoryImpl& operator = (const DialogFactoryImpl&);
  ///Private Destructor
  virtual ~DialogFactoryImpl();
};

#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<DialogFactoryImpl>;
#endif /* _WIN32 */
  /// The specific instantiation of the templated type
  typedef EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<DialogFactoryImpl> DialogFactory;

}
}

#endif //MANTIDQT_API_DIALOGFACTORY_H_
