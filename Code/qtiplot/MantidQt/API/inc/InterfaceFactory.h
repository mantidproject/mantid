#ifndef MANTIDQT_API_INTERFACEFACTORY_H_
#define MANTIDQT_API_INTERFACEFACTORY_H_

//------------------------
// Includes
//------------------------
#include "DllOption.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"
#include <QHash>
#include <QStringList>

namespace MantidQt
{

namespace API
{

//-------------------------------
// MantidQt forward declarations
//-------------------------------
class AlgorithmDialog;
class UserSubWindow;

/** 
    The AlgorithmDialogFactory is responsible for creating concrete instances of
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
class EXPORT_OPT_MANTIDQT_API AlgorithmDialogFactoryImpl : public Mantid::Kernel::DynamicFactory<AlgorithmDialog>
{

public:
  // Unhide the inherited create method
  using Mantid::Kernel::DynamicFactory<AlgorithmDialog>::createUnwrapped;

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmDialogFactoryImpl>;

  /// Private Constructor for singleton class
  AlgorithmDialogFactoryImpl() 
  {
  }
  /// Private copy constructor - NO COPY ALLOWED
  AlgorithmDialogFactoryImpl(const AlgorithmDialogFactoryImpl&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  AlgorithmDialogFactoryImpl& operator = (const AlgorithmDialogFactoryImpl&);
  ///Private Destructor
  virtual ~AlgorithmDialogFactoryImpl()
  {
  }
};

#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<AlgorithmDialogFactoryImpl>;
#endif /* _WIN32 */
  /// The specific instantiation of the templated type
  typedef EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<AlgorithmDialogFactoryImpl> AlgorithmDialogFactory;


/** 
    The UserSubWindowFactory is responsible for creating concrete instances of
    user interface classes. It is implemented as a singleton class.
    
    @author Martyn Gigg, Tessella plc
    @date 06/07/2010
    
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
class EXPORT_OPT_MANTIDQT_API UserSubWindowFactoryImpl : public Mantid::Kernel::DynamicFactory<UserSubWindow>
{
public:
  // Register an interface
  template<class TYPE>
  void subscribe()
  {
    Mantid::Kernel::Instantiator<TYPE, UserSubWindow> * allocator = new Mantid::Kernel::Instantiator<TYPE, UserSubWindow>();
    MantidQt::API::UserSubWindow *userInterface = allocator->createUnwrappedInstance();
    delete allocator;
    if( userInterface )
    {
      std::string realName = userInterface->name().toStdString();
      Mantid::Kernel::DynamicFactory<UserSubWindow>::subscribe<TYPE>(realName);
      // Save alias names
      QSet<QString> aliases = userInterface->aliases();
      QSetIterator<QString> itr(aliases);
      while( itr.hasNext() )
      {
        QString alias = itr.next();
        if( m_aliasLookup.contains(alias) )
        {
          if( m_badAliases.contains(alias) )
          {
            QList<std::string> names = m_badAliases.value(alias);
            names.append(realName);
            m_badAliases[alias] = names;
          }
          else
          {
            QList<std::string> names;
            names.append(m_aliasLookup.value(alias));
            names.append(realName);            
            m_badAliases.insert(alias, names);
          }
          continue;
        }
        m_aliasLookup.insert(alias, realName);
      }
    }
    else
    {
      throw Mantid::Kernel::Exception::NullPointerException("UserSubWindowFactoryImpl::subscribe", typeid(TYPE).name());
    }
  }
public:
  // Override createUnwrapped to search through the alias list
  UserSubWindow * createUnwrapped(const std::string & name) const;
  
protected:
  // Unhide the inherited create method
  using Mantid::Kernel::DynamicFactory<UserSubWindow>::createUnwrapped;
 
private:
  friend struct Mantid::Kernel::CreateUsingNew<UserSubWindowFactoryImpl>;

  /// Private Constructor for singleton class
  UserSubWindowFactoryImpl();
  /// Private copy constructor - NO COPY ALLOWED
  UserSubWindowFactoryImpl(const UserSubWindowFactoryImpl&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  UserSubWindowFactoryImpl& operator = (const UserSubWindowFactoryImpl&);
  ///Private Destructor
  virtual ~UserSubWindowFactoryImpl() {}
  /// Try to create a sub window from the list of aliases for an interface
  UserSubWindow * createFromAlias(const std::string & name) const;

private:
  /// A map of alias names to "real" names
  QHash<QString, std::string> m_aliasLookup;
  /// An index of multiply defined aliases
  QHash<QString, QList<std::string> > m_badAliases; 
  Mantid::Kernel::Logger & g_log;
};

#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<UserSubWindowFactoryImpl>;
#endif /* _WIN32 */
  /// The specific instantiation of the templated type
  typedef EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<UserSubWindowFactoryImpl> UserSubWindowFactory;


}
}

#endif //MANTIDQT_API_INTERFACEFACTORY_H_
