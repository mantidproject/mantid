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

using ReflCommand_uptr = std::unique_ptr<ReflCommand>;

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
  ReflCommandAdapter(QMenu *menu, ReflCommand_uptr adaptee)
      : m_adaptee(std::move(adaptee)) {

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
            submenu, std::move(ch)));
      }
    } else {
      // We are dealing with an action
      QAction *action =
          new QAction(QString::fromStdString(m_adaptee->name()), this);
      action->setIcon(QIcon(QString::fromStdString(m_adaptee->icon())));
      action->setSeparator(m_adaptee->isSeparator());
      menu->addAction(action);
      connect(action, SIGNAL(triggered()), this, SLOT(call()));
    }
  };

public slots:
  void call() { m_adaptee->execute(); }

private:
  // The adaptee
  ReflCommand_uptr m_adaptee;
  std::vector<std::unique_ptr<ReflCommandAdapter>> m_adapter;
};

typedef std::unique_ptr<ReflCommandAdapter> ReflCommandAdapter_uptr;
}
}
#endif /*MANTID_CUSTOMINTERFACES_REFLCOMMANDADAPTER_H*/