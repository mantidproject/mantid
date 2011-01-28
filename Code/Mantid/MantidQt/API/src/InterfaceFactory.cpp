//-----------------------------------------------
// Includes
//-----------------------------------------------
#include "MantidQtAPI/InterfaceFactory.h"
#include "MantidQtAPI/UserSubWindow.h"

using namespace MantidQt::API;

//*********************************************************
//                 UserSubWindow 
//*********************************************************

//----------------------------------------
// Public member functions
//----------------------------------------

/**
 * Create a raw pointer to the interface with the given name
 * @param name :: The name of the interface that should have been registered into the factory
 */
UserSubWindow * UserSubWindowFactoryImpl::createUnwrapped(const std::string & name) const
{
  // Try primary name as a start
  UserSubWindow *window;
  try
  {
    window = Mantid::Kernel::DynamicFactory<UserSubWindow>::createUnwrapped(name);
  }
  catch(Mantid::Kernel::Exception::NotFoundError&)
  {
    g_log.debug() << "\"" << name << "\" not registered as a real name, trying an alias.\n"; 
    window = NULL;
  }
  if( !window )
  {
    window = createFromAlias(name);
  }
  if( !window ) 
  {
    g_log.error() << "UserSubWindowFactory: \""+ name + "\" is not registered as an interface name.\n";
    throw Mantid::Kernel::Exception::NotFoundError("UserSubWindowFactory:"+ name + " is not registered or recognised as an alias of a known interface.\n", name);
  }
  return window;   
}

//----------------------------------------
// Public member functions
//----------------------------------------

/// Default constructor
UserSubWindowFactoryImpl::UserSubWindowFactoryImpl() : m_aliasLookup(), m_badAliases(), g_log(Mantid::Kernel::Logger::get("UserSubWindowFactory"))
{
}

/**
 * Create a user sub window by searching for an alias name
 * @param name :: The alias name to use to try and create an interface
 * @returns A pointer to a created interface pointer if this alias exists and is not multiply defined
 */
UserSubWindow * UserSubWindowFactoryImpl::createFromAlias(const std::string & name) const
{
  QString alias = QString::fromStdString(name);
  if( m_badAliases.contains(alias) )
  {
    std::string error = "Alias \"" + name + "\" is defined for multiple real interfaces: \"";
    QListIterator<std::string> itr(m_badAliases.value(alias));
    while( itr.hasNext() )
    {
      error += itr.next();
      if( itr.hasNext() )
      {
        error += ",";
      }
    }
    g_log.error() << error << "\"\n";
    return NULL;
  }

  if( m_aliasLookup.contains(alias) )
  {
    return this->createUnwrapped(m_aliasLookup.value(alias));
  }
  else
  {
    return NULL;
  }
}	


