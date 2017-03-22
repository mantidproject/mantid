#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H

#include "MantidKernel/make_unique.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommand.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <QObject>
#include <memory>
#include <qmenu.h>
#include <qtoolbar.h>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

using DataProcessorCommand_uptr = std::unique_ptr<DataProcessorCommand>;

/** @class DataProcessorCommandAdapter

DataProcessorCommandAdapter is an adapter that allows DataProcessorCommands to
be treated as
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DataProcessorCommandAdapter
    : public QObject {
  Q_OBJECT
public:
  /** Constructor: Adds actions to a menu
  * @param menu :: The menu where the actions will be added
  * @param adaptee :: The action to add
  */
  DataProcessorCommandAdapter(QMenu *menu, DataProcessorCommand_uptr adaptee)
      : m_adaptee(std::move(adaptee)) {

    if (m_adaptee->hasChild()) {
      // We are dealing with a submenu
      // Add the submenu
      QMenu *submenu =
          menu->addMenu(QIcon(m_adaptee->icon()), m_adaptee->name());
      // Add the actions
      auto &child = m_adaptee->getChild();
      for (auto &ch : child) {
        m_adapter.push_back(
            Mantid::Kernel::make_unique<DataProcessorCommandAdapter>(
                submenu, std::move(ch)));
      }
    } else {
      // We are dealing with an action
      QAction *action = getAction();
      menu->addAction(action);
    }
  };

  /** Constructor: Adds actions to a toolbar
  * @param toolbar :: The toolbar where actions will be added
  * @param adaptee :: The action to add
  */
  DataProcessorCommandAdapter(QToolBar *toolbar,
                              DataProcessorCommand_uptr adaptee)
      : m_adaptee(std::move(adaptee)) {

    if (!m_adaptee->hasChild()) {
      // Sub-menus cannot be added to a toolbar

      QAction *action = getAction(true);
      toolbar->addAction(action);
    }
  };

  /**
   * Returns the action
   *
   * @param shortcut : Whether or not to add a shortcut
   */
  QAction *getAction(bool shortcut = false) {
    QAction *action = new QAction(m_adaptee->name(), this);
    action->setIcon(QIcon(m_adaptee->icon()));
    action->setSeparator(m_adaptee->isSeparator());
    action->setToolTip(m_adaptee->tooltip());
    action->setWhatsThis(m_adaptee->whatsthis());
    if (shortcut)
      action->setShortcut(QKeySequence(m_adaptee->shortcut()));
    connect(action, SIGNAL(triggered()), this, SLOT(call()));

    return action;
  };

public slots:
  void call() { m_adaptee->execute(); }

private:
  // The adaptee
  DataProcessorCommand_uptr m_adaptee;
  std::vector<std::unique_ptr<DataProcessorCommandAdapter>> m_adapter;
};

typedef std::unique_ptr<DataProcessorCommandAdapter>
    DataProcessorCommandAdapter_uptr;
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H*/