// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------
// Includes
//-----------------------------------------------
#include "MantidQtWidgets/Common/UserSubWindowFactory.h"
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
QSet<QString>
UserSubWindowFactoryImpl::categories(const QString &interfaceName) const {
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

/**
 * The keys associated with UserSubWindow classes
 * @returns A QStringList containing the keys from the UserSubWindowFactory that
 * refer to UserSubWindow classes
 */
QStringList UserSubWindowFactoryImpl::keys() const {
  QStringList key_list;
  const auto keys = getKeys();
  for (const auto &key : keys) {
    key_list.append(QString::fromStdString(key));
  }
  return key_list;
}
