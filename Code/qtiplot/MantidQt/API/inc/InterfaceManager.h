#ifndef MANTIDQT_API_DIALOGMANAGER_H_
#define MANTIDQT_API_DIALOGMANAGER_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "MantidKernel/SingletonHolder.h"

#include <QString>

//----------------------------------
// Qt Forward declarations
//----------------------------------
class QWidget;

//----------------------------------
// Mantid forward declarations
//----------------------------------
namespace Mantid 
{
namespace API
{
  class IAlgorithm;
}

namespace Kernel
{
  class Logger;
}
}

// Top level namespace for this library
namespace MantidQt
{

namespace API
{

//----------------------------------
// Forward declarations
//----------------------------------
class AlgorithmDialog;
class UserSubWindow;

/** 
    This class is responsible for creating the correct dialog for an algorithm. If 
    no specialized version is registered for that algorithm then the default is created
    
    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class EXPORT_OPT_MANTIDQT_API InterfaceManagerImpl
{

public:
  
  /// Create a new instance of the correct type of AlgorithmDialog
  AlgorithmDialog* createDialog(Mantid::API::IAlgorithm* alg, QWidget* parent = 0,
				bool forScript = false, const QString & preset_values = QString(), 
				const QString & optional_msg = QString(), const QString & enabled_names = QString());
				
  /// Create a new instance of the correct type of UserSubWindow
  UserSubWindow* createSubWindow(const QString & interface_name, QWidget* parent = 0);

  /// The keys associated with UserSubWindow classes
  QStringList getUserSubWindowKeys() const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<InterfaceManagerImpl>;
  
  ///Private Constructor
  InterfaceManagerImpl();
  /// Private copy constructor - NO COPY ALLOWED
  InterfaceManagerImpl(const InterfaceManagerImpl&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  InterfaceManagerImpl& operator = (const InterfaceManagerImpl&);
  ///Private Destructor
  virtual ~InterfaceManagerImpl();

  //A static reference to the Logger
  static Mantid::Kernel::Logger & g_log;
};

#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<InterfaceManagerImpl>;
#endif /* _WIN32 */
/// The specific instantiation of the templated type
typedef EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<InterfaceManagerImpl> InterfaceManager;

}
}

#endif //MANTIDQT_API_DIALOGMANAGER
