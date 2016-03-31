#ifndef MANTID_CUSTOMINTERFACES_REFLCOMMANDADAPTER_H
#define MANTID_CUSTOMINTERFACES_REFLCOMMANDADAPTER_H

#include "MantidKernel/make_unique.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCommand.h"
#include <QObject>
#include <memory>
#include <qmenu.h>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
/** @class ReflCommandAdapter

ReflCommandAdapter is an adapter that allows ReflCommands to be treated as
QObjects for signals.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class ReflCommandAdapter : public QObject {
  Q_OBJECT
public:
  ReflCommandAdapter(QMenu *menu, ReflCommand_sptr adaptee)
      : m_adaptee(adaptee) {

    if (m_adaptee->hasChild()) {
      // We are dealing with a submenu
      // Add the submenu
      QMenu *submenu =
          menu->addMenu(QIcon(QString::fromStdString(m_adaptee->icon())),
                        QString::fromStdString(m_adaptee->name()));
      // Add the actions
      auto &child = m_adaptee->getChild();
      for (auto &ch : child) {
        m_adapter.push_back(Mantid::Kernel::make_unique<ReflCommandAdapter>(
            submenu, std::shared_ptr<ReflCommand>(ch.get())));
      }
    } else {
      // We are dealing with an action
      addAction(menu, m_adaptee);
    }
  };
public slots:
  void call() { m_adaptee->execute(); }

private:
  /** Adds an action to a menu
  * @param menu : [input] The menu that will contain the action
  * @param adaptee : [input] The adaptee
  */
  void addAction(QMenu *menu, ReflCommand_sptr adaptee) {
    QAction *action =
        new QAction(QString::fromStdString(adaptee->name()), this);
    action->setIcon(QIcon(QString::fromStdString(adaptee->icon())));
    action->setSeparator(adaptee->isSeparator());
    menu->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(call()));
  };
  // The adaptee
  ReflCommand_sptr m_adaptee;
  std::vector<std::unique_ptr<ReflCommandAdapter>> m_adapter;
};

typedef std::unique_ptr<ReflCommandAdapter> ReflCommandAdapter_uptr;
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLCOMMANDADAPTER_H*/