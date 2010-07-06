//-----------------------------------------------
// Includes
//-----------------------------------------------
#include "MantidQtAPI/InterfaceFactory.h"


using namespace MantidQt::API;

//----------------- UserSubWindow --------------------------

//----------------------------------------------------------
// Public member functions
//----------------------------------------------------------

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
    window = NULL;
  }
  if( !window )
  {
    window = createFromAlias(name);
  }
  if( !window ) 
  {
    throw Mantid::Kernel::Exception::NotFoundError("UserSubWindowFactory:"+ name + " is not registered or recognised as an alias of a known interface.\n", name);
  }
  return window;   
}

//----------------------------------------------------------
// Private member functions
//----------------------------------------------------------

/// Default constructor
UserSubWindowFactoryImpl::UserSubWindowFactoryImpl() : m_aliasLookup(), m_badAliases(), g_log(Mantid::Kernel::Logger::get("UserSubWindowFactory"))
{
}

/**
 * Create a user sub window by searching for an alias name
 * @param name The alias name to use to try and create an interface
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


