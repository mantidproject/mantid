// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDQT_API_USERSUBWINDOWFACTORYIMPL_H_
#define MANTIDQT_API_USERSUBWINDOWFACTORYIMPL_H_

//------------------------
// Includes
//------------------------
#include "DllOption.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include <QHash>
#include <QSetIterator>
#include <QStringList>
#include <set>

namespace MantidQt {

namespace API {

//-------------------------------
// MantidQt forward declarations
//-------------------------------
class AlgorithmDialog;
class UserSubWindow;

/**
    The UserSubWindowFactory is responsible for creating concrete instances of
    user interface classes. It is implemented as a singleton class.

    @author Martyn Gigg, Tessella plc
    @date 06/07/2010
*/
class EXPORT_OPT_MANTIDQT_COMMON UserSubWindowFactoryImpl
    : public Mantid::Kernel::DynamicFactory<UserSubWindow> {
public:
  UserSubWindowFactoryImpl(const UserSubWindowFactoryImpl &) = delete;
  UserSubWindowFactoryImpl &
  operator=(const UserSubWindowFactoryImpl &) = delete;
  // Override createUnwrapped to search through the alias list
  UserSubWindow *createUnwrapped(const std::string &name) const override;

  QSet<QString> categories(const QString &interfaceName) const;

  QStringList keys() const;

  template <typename TYPE> void subscribe() {
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

protected:
  // Unhide the inherited create method
  using Mantid::Kernel::DynamicFactory<UserSubWindow>::createUnwrapped;

private:
  friend struct Mantid::Kernel::CreateUsingNew<UserSubWindowFactoryImpl>;

  /// Private Constructor for singleton class
  UserSubWindowFactoryImpl();
  /// Private Destructor
  ~UserSubWindowFactoryImpl() override = default;
  /// Try to create a sub window from the list of aliases for an interface
  UserSubWindow *createFromAlias(const std::string &name) const;

  /// Save the list of aliases
  /**
   * Save the alias names of an interface
   * @param realName :: The real name of the interface
   */
  template <typename TYPE> void saveAliasNames(const std::string &realName) {
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

private:
  /// A map of alias names to "real" names
  QHash<QString, std::string> m_aliasLookup;
  /// An index of multiply defined aliases
  QHash<QString, QList<std::string>> m_badAliases;
  /// A map of interfaces to their categories.
  QHash<QString, QSet<QString>> m_categoryLookup;
};

/// The specific instantiation of the templated type
using UserSubWindowFactory =
    Mantid::Kernel::SingletonHolder<UserSubWindowFactoryImpl>;
} // namespace API
} // namespace MantidQt

namespace Mantid {
namespace Kernel {
EXTERN_MANTIDQT_COMMON template class EXPORT_OPT_MANTIDQT_COMMON
    Mantid::Kernel::SingletonHolder<MantidQt::API::UserSubWindowFactoryImpl>;
} // namespace Kernel
} // namespace Mantid

#endif
