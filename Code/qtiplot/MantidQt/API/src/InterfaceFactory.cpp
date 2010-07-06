//-----------------------------------------------
// Includes
//-----------------------------------------------
#include "MantidQtAPI/InterfaceFactory.h"
#include "MantidQtAPI/UserSubWindow.h"

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
 * Returns the name of the interface
 * @param window A pointer (not NULL) to a UserSubWindow class
 * @returns A string containing the interface name
 */
std::string UserSubWindowFactoryImpl::getInterfaceName(UserSubWindow *window) const
{
  return window->name().toStdString();
}

/**
 * Save the alias names of the interface
 * @param userInterface A pointer (not NULL) to a UserSubWindow class
 */
void UserSubWindowFactoryImpl::saveAliasNames(UserSubWindow *userInterface)
{
  std::string realName = userInterface->name().toStdString();
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

/**
 * Delete the user interface object given
 * @param userInterface A pointer to the user interface
 */
void UserSubWindowFactoryImpl::deleteTemporaryInterface(UserSubWindow *userInterface) const
{
  userInterface->deleteLater();
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


