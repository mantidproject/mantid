#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H

#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QObject>
#include <QMenu>
#include <QToolBar>
#include <memory>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

using Command_uptr = std::unique_ptr<Command>;

/** @class QtCommandAdapter

QtCommandAdapter is an adapter that allows Commands to
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
class EXPORT_OPT_MANTIDQT_COMMON QtCommandAdapter : public QObject {
  Q_OBJECT
public:
  /** Constructor: Adds actions to a menu
  * @param menu :: The menu where the actions will be added
  * @param adaptee :: The action to add
  */
  QtCommandAdapter(QMenu *menu, Command_uptr adaptee)
      : m_action(nullptr), m_adaptee(std::move(adaptee)) {

    if (m_adaptee->hasChild()) {
      // We are dealing with a submenu
      // Add the submenu
      QMenu *submenu =
          menu->addMenu(QIcon(m_adaptee->icon()), m_adaptee->name());
      // Add the actions
      auto &child = m_adaptee->getChild();
      for (auto &ch : child) {
        m_adapter.push_back(Mantid::Kernel::make_unique<QtCommandAdapter>(
            submenu, std::move(ch)));
      }
    } else {
      // We are dealing with an action
      /// @todo Generalise m_action to include submenus
      m_action = createAction();
      menu->addAction(m_action);
    }
  };

  /** Constructor: Adds actions to a toolbar
  * @param toolbar :: The toolbar where actions will be added
  * @param adaptee :: The action to add
  */
  QtCommandAdapter(QToolBar *toolbar, Command_uptr adaptee)
      : m_action(nullptr), m_adaptee(std::move(adaptee)) {

    if (!m_adaptee->hasChild()) {
      // Sub-menus cannot be added to a toolbar

      m_action = createAction(true);
      toolbar->addAction(m_action);
    }
  };

  /** Set the action to be enabled/disabled according
   * to whether processing is running or not based on
   * the properties of the adaptee.
   *
   * @param isProcessing :: true if processing is in progress
   * */
  void updateEnabledState(const bool isProcessing) {
    if (!m_action)
      return;

    // If the command modifies settings, always disable it
    // when processing is in progress, and enable it when idle.
    // Otherwise, if the command modifies running processes,
    // enable it if processing is in progress.
    if (m_adaptee->modifiesSettings())
      m_action->setEnabled(!isProcessing);
    else if (m_adaptee->modifiesRunningProcesses())
      m_action->setEnabled(isProcessing);
  }

  QAction *getAction() {
    return m_action;
  }

private:
  /**
   * Creates the action
   *
   * @param shortcut : Whether or not to add a shortcut
   */
  QAction *createAction(bool shortcut = false) {
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
  QAction *m_action;
  // The adaptee
  Command_uptr m_adaptee;
  std::vector<std::unique_ptr<QtCommandAdapter>> m_adapter;
};

using QtCommandAdapter_uptr = std::unique_ptr<QtCommandAdapter>;
}
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H*/
