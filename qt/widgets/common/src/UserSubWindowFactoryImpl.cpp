// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------
// Includes
//-----------------------------------------------
#include "MantidQtWidgets/Common/UserSubWindowFactoryImpl.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include <sstream>

using namespace MantidQt::API;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("UserSubWindowFactoryImpl");
} // namespace

//*********************************************************
//                 UserSubWindow
//*********************************************************

//----------------------------------------
// Public member functions
//----------------------------------------

/**
 * Create a raw pointer to the interface with the given name
 * @param name :: The name of the interface that should have been registered
 * into the factory
 */
UserSubWindow *
UserSubWindowFactoryImpl::createUnwrapped(const std::string &name) const {
  // Try primary name as a start
  UserSubWindow *window;
  try {
    window =
        Mantid::Kernel::DynamicFactory<UserSubWindow>::createUnwrapped(name);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    g_log.debug() << "\"" << name
                  << "\" not registered as a real name, trying an alias.\n";
    window = nullptr;
  }
  if (!window) {
    window = createFromAlias(name);
  }
  if (!window) {
    g_log.error() << "UserSubWindowFactory: \"" + name +
                         "\" is not registered as an interface name.\n";
    throw Mantid::Kernel::Exception::NotFoundError(
        "UserSubWindowFactory:" + name +
            " is not registered or recognised as "
            "an alias of a known interface.\n",
        name);
  }
  return window;
}

/**
 * Return the set of categories that the interface with the given name belongs
 *to.
 *
 * @param interfaceName :: The name of the interface.
 * @returns the set of category names if an interface with the given name has
 *been registered,
 *          else an empty set.
 */
QSet<QString> UserSubWindowFactoryImpl::getInterfaceCategories(
    const QString &interfaceName) const {
  if (!m_categoryLookup.contains(interfaceName))
    return QSet<QString>();

  return m_categoryLookup[interfaceName];
}

//----------------------------------------
// Public member functions
//----------------------------------------

/// Default constructor
UserSubWindowFactoryImpl::UserSubWindowFactoryImpl()
    : m_aliasLookup(), m_badAliases() {}

/**
 * Create a user sub window by searching for an alias name
 * @param name :: The alias name to use to try and create an interface
 * @returns A pointer to a created interface pointer if this alias exists and is
 * not multiply defined
 */
UserSubWindow *
UserSubWindowFactoryImpl::createFromAlias(const std::string &name) const {
  QString alias = QString::fromStdString(name);
  if (m_badAliases.contains(alias)) {
    std::string error =
        "Alias \"" + name + "\" is defined for multiple real interfaces: \"";
    QListIterator<std::string> itr(m_badAliases.value(alias));
    while (itr.hasNext()) {
      error += itr.next();
      if (itr.hasNext()) {
        error += ",";
      }
    }
    g_log.error() << error + "\n";
    return nullptr;
  }

  if (m_aliasLookup.contains(alias)) {
    return this->createUnwrapped(m_aliasLookup.value(alias));
  } else {
    return nullptr;
  }
}

template <typename TYPE> void UserSubWindowFactoryImpl::subscribe() {
  std::string realName = TYPE::name();
  Mantid::Kernel::DynamicFactory<UserSubWindow>::subscribe<TYPE>(realName);
  saveAliasNames<TYPE>(realName);

  // Make a record of each interface's categories.
  const QStringList categories =
      TYPE::categoryInfo().split(";", QString::SkipEmptyParts);
  QSet<QString> result;
  foreach (const QString category, categories) {
    result.insert(category.trimmed());
  }
  m_categoryLookup[QString::fromStdString(realName)] = result;
}

/**
 * Save the alias names of an interface
 * @param realName :: The real name of the interface
 */
template <typename TYPE>
void UserSubWindowFactoryImpl::saveAliasNames(const std::string &realName) {
  std::set<std::string> aliases = TYPE::aliases();
  for (const auto &alias_std_str : aliases) {
    QString alias = QString::fromStdString(alias_std_str);
    if (m_aliasLookup.contains(alias)) {
      if (m_badAliases.contains(alias)) {
        QList<std::string> names = m_badAliases.value(alias);
        names.append(realName);
        m_badAliases[alias] = names;
      } else {
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
 * The keys associated with UserSubWindow classes
 * @returns A QStringList containing the keys from the UserSubWindowFactory that
 * refer to UserSubWindow classes
 */
QStringList UserSubWindowFactoryImpl::getUserSubWindowKeys() const {
  QStringList key_list;
  const auto keys = getKeys();
  for (const auto &key : keys) {
    key_list.append(QString::fromStdString(key));
  }
  return key_list;
}