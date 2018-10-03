// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ManageCustomMenus.h"
#include "../ApplicationWindow.h"

#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"

#include <QtGui>

using MantidQt::API::MantidDesktopServices;

/**
 * Constructor for object. Performs initial setup and calls subsequent setup
 * functions.
 * @param parent :: pointer to the main MantidPlot ApplicationWindow object
 */
ManageCustomMenus::ManageCustomMenus(QWidget *parent)
    : QDialog(parent), m_scriptsTree(nullptr), m_menusTree(nullptr) {
  m_uiForm.setupUi(this);
  m_appWindow = static_cast<ApplicationWindow *>(parent);
  initLayout();
}
/**
 * Makes signal/slot connections and small changes to interface which QtDesigner
 * does not give access to.
 */
void ManageCustomMenus::initLayout() {
  m_scriptsTree = m_uiForm.twScripts;
  m_menusTree = m_uiForm.twMenus;

  m_scriptsTree->setHeaderLabel("Python Scripts");
  m_menusTree->setHeaderLabel("Custom Menus");

  // create qt connections
  connect(m_uiForm.pbAddScript, SIGNAL(clicked()), this,
          SLOT(addScriptClicked()));
  connect(m_uiForm.pbRemoveScript, SIGNAL(clicked()), this,
          SLOT(remScriptClicked()));
  connect(m_uiForm.pbAddItem, SIGNAL(clicked()), this, SLOT(addItemClicked()));
  connect(m_uiForm.pbRemoveItem, SIGNAL(clicked()), this,
          SLOT(remItemClicked()));
  connect(m_uiForm.pbAddMenu, SIGNAL(clicked()), this, SLOT(addMenuClicked()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(close()));
  // Perform subsequent setups
  populateMenuTree();
}
/**
 * Populates the m_menusTree to reflect the currently set custom menus.
 */
void ManageCustomMenus::populateMenuTree() {
  m_menusTree->clear();
  m_widgetMap.clear();

  QListIterator<QMenu *> mItr(m_appWindow->getCustomMenus());
  while (mItr.hasNext()) {
    QMenu *customMenu = mItr.next();
    QTreeWidgetItem *menu =
        new QTreeWidgetItem(QStringList(customMenu->title()));
    m_widgetMap.insert(menu, customMenu);
    menu->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
                   Qt::ItemIsEditable);
    QList<QAction *> scripts = customMenu->actions();
    QListIterator<QAction *> kItr(scripts);
    while (kItr.hasNext()) {
      QAction *action = kItr.next();
      QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(action->text()));
      m_widgetMap.insert(item, action);
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
                     Qt::ItemIsEditable);
      item->setData(0, Qt::UserRole, action->data().toString());
      item->setToolTip(0, action->data().toString());
      menu->addChild(item);
    }
    m_menusTree->addTopLevelItem(menu);
  }
}

/**
 * Returns a list of pointers to the selected items in the Scripts and Custom
 * Interfaces trees.
 * @return list of selected items
 */
QList<QTreeWidgetItem *> ManageCustomMenus::getCurrentSelection() {
  QList<QTreeWidgetItem *> result = m_scriptsTree->selectedItems();
  return result;
}
/**
 * Returns pointer to currently selected menu item.
 * @return pointer to currently selected menu item
 */
QTreeWidgetItem *ManageCustomMenus::getCurrentMenuSelection() {
  QTreeWidgetItem *result = m_menusTree->currentItem();
  return result;
}
/**
 * Handles adding a script to the scripts tree, through a FileDialog.
 */
void ManageCustomMenus::addScriptClicked() {
  QString scriptsDir = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "pythonscripts.directories"));
  QStringList scriptFiles = QFileDialog::getOpenFileNames(
      this, "Select Python Files", scriptsDir, "Python (*.py)");
  if (!scriptFiles.isEmpty()) {
    // Add file items to m_scriptsTree
    QStringList::const_iterator itEnd = scriptFiles.constEnd();
    for (QStringList::const_iterator itr = scriptFiles.constBegin();
         itr != itEnd; ++itr) {
      QString suggestedName = QFileInfo(*itr).baseName();
      if (!m_scriptsTree
               ->findItems(suggestedName,
                           Qt::MatchFixedString | Qt::MatchCaseSensitive)
               .isEmpty())
        continue;

      QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(suggestedName));
      item->setData(0, Qt::UserRole, *itr);
      item->setToolTip(0, *itr);
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
                     Qt::ItemIsEditable);
      m_scriptsTree->insertTopLevelItem(0, item);
    }
  }
}
/**
 * Handles removing selected scripts from the m_scriptsTree window.
 */
void ManageCustomMenus::remScriptClicked() {
  if (m_scriptsTree->selectedItems().isEmpty()) {
    QMessageBox::information(
        this, "MantidPlot",
        "No item selected - please select a script from the left-hand list.");
  } else {
    foreach (QTreeWidgetItem *item, m_scriptsTree->selectedItems()) {
      delete item;
    }
  }
}
/**
 * Adds item (script or custom interface) to custom menu. Removes added scripts
 * from the scripts tree.
 */
void ManageCustomMenus::addItemClicked() {
  QList<QTreeWidgetItem *> selection = getCurrentSelection();
  QTreeWidgetItem *menu = getCurrentMenuSelection();
  if (selection.isEmpty()) {
    QMessageBox::information(
        this, "MantidPlot",
        "No item selected - please select a script in the left-hand list of "
        "scripts.\n"
        "If none are listed, use the 'Add Script' button to add some files.");
  } else if (menu == nullptr) {
    QMessageBox::information(this, "MantidPlot",
                             "No menu selected - please select a menu on the "
                             "right-hand side to which to add this script.\n"
                             "If no custom menus are present, use the 'Add "
                             "Menu' button to create one.");
  } else {
    // Ensure using top-level menu.
    if (menu->parent() != nullptr) {
      menu = menu->parent();
    }

    foreach (QTreeWidgetItem *item, selection) { // foreach is a Qt macro (
      // http://doc.qt.nokia.com/4.4/containers.html#the-foreach-keyword
      // )
      menu->addChild(item);
      QString menuName = menu->text(0);
      QString itemName = item->text(0);
      QString item_data = item->data(0, Qt::UserRole).toString();
      m_appWindow->addUserMenuAction(menuName, itemName, item_data);
    }
    // Refresh menu list
    populateMenuTree();

    // Remove scripts elements that have been added to the menu.
    if (!m_scriptsTree->selectedItems().isEmpty()) {
      remScriptClicked();
    }
  }
}
/**
 * Removes item from custom menu, or custom menu itself if selected.
 */
void ManageCustomMenus::remItemClicked() {
  QTreeWidgetItem *item = getCurrentMenuSelection();
  if (item == nullptr) {
    QMessageBox::information(this, "MantidPlot",
                             "No item selected - please "
                             "select a script or menu in "
                             "the right-hand list.");
  } else {
    if (item->parent() != nullptr) {
      // Delete menu sub-item
      QTreeWidgetItem *menu = item->parent();
      m_appWindow->removeUserMenuAction(menu->text(0), item->text(0));
    } else {
      // Delete menu
      m_appWindow->removeUserMenu(item->text(0));
    }
    // Refresh menu list
    populateMenuTree();
  }
}
/**
 * Adds new top-level menu to the interface.
 */
void ManageCustomMenus::addMenuClicked() {
  bool ok(false);
  QString name = QInputDialog::getText(
      this, "Create a Menu", "Menu name:", QLineEdit::Normal, "", &ok);
  if (ok) {
    if (m_menusTree
            ->findItems(name, Qt::MatchFixedString | Qt::MatchCaseSensitive)
            .isEmpty()) {
      if (!name.isEmpty()) {
        m_appWindow->addUserMenu(name);
        populateMenuTree();
      }
    } else {
      QMessageBox::information(this, "MantidPlot",
                               "A menu with that name already exists.");
    }
  }
}
/**
 * Opens web browser to wiki page for this dialog.
 */
void ManageCustomMenus::helpClicked() {
  QUrl helpUrl("http://www.mantidproject.org/ManageCustomMenus");
  MantidDesktopServices::openUrl(helpUrl);
}
