#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H

#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QMenu>
#include <QObject>
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

    if (m_adaptee->hasChildren()) {
      initializeSubmenu(menu);
    } else {
      // We are dealing with an action
      initializeAction(menu);
    }
  };

  /** Constructor: Adds actions to a toolbar
   * @param toolbar :: The toolbar where actions will be added
   * @param adaptee :: The action to add
   */
  QtCommandAdapter(QToolBar *toolbar, Command_uptr adaptee)
      : m_action(nullptr), m_adaptee(std::move(adaptee)) {

    // Sub-menus cannot be added to a toolbar
    if (m_adaptee->hasChildren())
      return;

    // We are dealing with an action
    initializeAction(toolbar, true);
  };

  /** Set the action to be enabled/disabled according
   * to whether processing is running or not based on
   * the properties of the adaptee.
   *
   * @param isProcessing :: true if processing is in progress
   * */
  void updateEnabledState(const bool isProcessing) {
    // Recurse through any child items
    for (auto &child : m_childAdapters)
      child->updateEnabledState(isProcessing);

    // Update the action (nothing to do if this is a submenu rather
    // than an action)
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

  bool hasAction() { return m_action != nullptr; }
  QAction *getAction() { return m_action.get(); }

private:
  /**
   * Creates a submenu
   *
   * @param menu : the parent menu to add the submenu to
   */
  void initializeSubmenu(QMenu *menu) {
    // We are dealing with a submenu
    // Add the submenu. Note that menu takes ownership of submenu.
    QMenu *submenu = menu->addMenu(QIcon(m_adaptee->icon()), m_adaptee->name());
    // Add the actions
    auto &children = m_adaptee->getChildren();
    std::transform(children.begin(), children.end(),
                   std::back_inserter(m_childAdapters),
                   [&submenu](Command_uptr &child) {
                     return Mantid::Kernel::make_unique<QtCommandAdapter>(
                         submenu, std::move(child));
                   });
  }

  /**
   * Creates an action and adds it to a widget
   *
   * @param widget : The widget to add the action to
   * @param shortcut : Whether or not to add a shortcut
   */
  void initializeAction(QWidget *widget, bool shortcut = false) {
    m_action = Mantid::Kernel::make_unique<QAction>(m_adaptee->name(), this);
    m_action->setIcon(QIcon(m_adaptee->icon()));
    m_action->setSeparator(m_adaptee->isSeparator());
    m_action->setToolTip(m_adaptee->tooltip());
    m_action->setWhatsThis(m_adaptee->whatsthis());
    if (shortcut)
      m_action->setShortcut(QKeySequence(m_adaptee->shortcut()));
    connect(m_action.get(), SIGNAL(triggered()), this, SLOT(call()));

    // The widget does not take ownership of the action so we retain ownership
    widget->addAction(m_action.get());
  };

public slots:
  void call() { m_adaptee->execute(); }

private:
  // The menu item's action. May be null for a parent menu item.
  std::unique_ptr<QAction> m_action;
  // The adaptee
  Command_uptr m_adaptee;
  // The child adaptors
  std::vector<std::unique_ptr<QtCommandAdapter>> m_childAdapters;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOMMANDADAPTER_H*/
