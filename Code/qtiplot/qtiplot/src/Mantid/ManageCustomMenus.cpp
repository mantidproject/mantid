#include "ManageCustomMenus.h"
#include "../ApplicationWindow.h"

#include "MantidQtAPI/InterfaceManager.h"
#include "MantidKernel/ConfigService.h"

#include <QtGui>
/**
* Constructor for object. Performs initial setup and calls subsequent setup functions.
* @param parent pointer to the main MantidPlot ApplicationWindow object
*/
ManageCustomMenus::ManageCustomMenus(QWidget *parent) : QDialog(parent),
m_scriptsTree(0), m_customInterfacesTree(0), m_menusTree(0)
{
	m_uiForm.setupUi(this);
    m_appWindow = static_cast<ApplicationWindow*>(parent);
    initLayout();
}
/**
* Makes signal/slot connections and small changes to interface which QtDesigner does not give access to.
*/
void ManageCustomMenus::initLayout()
{
    m_scriptsTree = m_uiForm.twScripts;
    m_customInterfacesTree = m_uiForm.twCustomInterfaces;
    m_menusTree = m_uiForm.twMenus;

    m_scriptsTree->setHeaderLabel("Python Scripts");
    m_customInterfacesTree->setHeaderLabel("Custom Interfaces");
    m_menusTree->setHeaderLabel("Custom Menus");

    // create qt connections
    connect(m_uiForm.pbAddScript, SIGNAL(clicked()), this, SLOT(addScriptClicked()));
    connect(m_uiForm.pbRemoveScript, SIGNAL(clicked()), this, SLOT(remScriptClicked()));
    connect(m_uiForm.pbAddItem, SIGNAL(clicked()), this, SLOT(addItemClicked()));
    connect(m_uiForm.pbRemoveItem, SIGNAL(clicked()), this, SLOT(remItemClicked()));
    connect(m_uiForm.pbAddMenu, SIGNAL(clicked()), this, SLOT(addMenuClicked()));   
    connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
    connect(m_uiForm.pbConfirm, SIGNAL(clicked()), this, SLOT(close()));
    // Perform subsequent setups
    getCustomInterfaceList();
    populateMenuTree();
}
/**
* Populates the m_menusTree to reflect the currently set custom menus.
*/
void ManageCustomMenus::populateMenuTree()
{
    m_menusTree->clear();
    m_widgetMap.clear();

    QListIterator<QMenu*> mItr(m_appWindow->getCustomMenus());
    while( mItr.hasNext() )
    {
        QMenu *customMenu = mItr.next();
        QTreeWidgetItem *menu = new QTreeWidgetItem(QStringList(customMenu->title()));
        m_widgetMap.insert(menu, customMenu);
        menu->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
        QList<QAction*> scripts = customMenu->actions();
        QListIterator<QAction*> kItr(scripts);
        while( kItr.hasNext() )
        {
            QAction *action = kItr.next();
            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(action->text()));
            m_widgetMap.insert(item, action);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
            item->setData(0, Qt::UserRole, action->data().toString());
            item->setToolTip(0, action->data().toString());
            menu->addChild(item);
        }
        m_menusTree->addTopLevelItem(menu);    
    }
}
/**
* Gets the list of Custom Interfaces that have been registered with Mantid.
*/
void ManageCustomMenus::getCustomInterfaceList()
{
    QStringList user_windows = MantidQt::API::InterfaceManager::Instance().getUserSubWindowKeys();
    QStringListIterator itr(user_windows);
    while( itr.hasNext() )
    {
        QString name = itr.next();
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(name));
        item->setData(0, Qt::UserRole, name);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
        m_customInterfacesTree->addTopLevelItem(item);
    }
}
/**
* Returns a list of pointers to the selected items in the Scripts and Custom Interfaces trees.
* @return list of selected items
*/
QList<QTreeWidgetItem*> ManageCustomMenus::getCurrentSelection()
{
    QList<QTreeWidgetItem*> result;
    result = m_scriptsTree->selectedItems() + m_customInterfacesTree->selectedItems();
    return result;
}
/**
* Returns pointer to currently selected menu item.
* @return pointer to currently selected menu item
*/
QTreeWidgetItem* ManageCustomMenus::getCurrentMenuSelection()
{
    QTreeWidgetItem* result = 0;
    result = m_menusTree->currentItem();
    return result;
}
/**
* Handles adding a script to the scripts tree, through a FileDialog.
*/
void ManageCustomMenus::addScriptClicked()
{
    QString scriptsDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directories"));
    QStringList scriptFiles = QFileDialog::getOpenFileNames(this, "Select Python Files", scriptsDir, "Python (*.py)");
    if ( !scriptFiles.isEmpty() )
    {
        // Add file items to m_scriptsTree
        QStringList::const_iterator itEnd = scriptFiles.constEnd();
        for( QStringList::const_iterator itr = scriptFiles.constBegin(); itr != itEnd; ++itr )
        {
            QString suggestedName = QFileInfo(*itr).baseName();
            if( !m_scriptsTree->findItems(suggestedName, Qt::MatchFixedString | Qt::MatchCaseSensitive).isEmpty() ) continue;

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(suggestedName));
            item->setData(0, Qt::UserRole, *itr);
            item->setToolTip(0, *itr);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
            m_scriptsTree->insertTopLevelItem(0, item);
        }
    }
}
/**
* Handles removing selected scripts from the m_scriptsTree window.
*/
void ManageCustomMenus::remScriptClicked()
{
    if ( m_scriptsTree->selectedItems().isEmpty() )
    {
        QMessageBox::information(this, "MantidPlot", "No item selected - please select a script from the left-hand list.");
    }
    else
    {
        QTreeWidgetItem* item;
        foreach(item, m_scriptsTree->selectedItems())
        {
            delete item;
        }
    }
}
/**
* Adds item (script or custom interface) to custom menu. Removes added scripts from the scripts tree.
*/
void ManageCustomMenus::addItemClicked()
{
    QList<QTreeWidgetItem*> selection = getCurrentSelection();
    QTreeWidgetItem* menu = getCurrentMenuSelection();
    if ( selection.isEmpty() )
    {
        QMessageBox::information(this, "MantidPlot", "No item selected - please select a script in the left-hand list of scripts.\n"
                                                     "If none are listed, use the 'Add Script' button to add some files.");
    }
    else if ( menu == 0 )
    {
        QMessageBox::information(this, "MantidPlot", "No menu selected - please select a menu on the right-hand side to which to add this script.\n"
            "If no custom menus are present, use the 'Add Menu' button to create one.");
    }
    else
    {
        // Ensure using top-level menu.
        if ( menu->parent() != 0 )
        {
            menu = menu->parent();
        }

        QTreeWidgetItem* item;
        foreach(item, selection)
        { // foreach is a Qt macro ( http://doc.qt.nokia.com/4.4/containers.html#the-foreach-keyword )
            menu->addChild(item);
            QString menuName = menu->text(0);
            QString itemName = item->text(0);
            QString item_data = item->data(0, Qt::UserRole).toString();
            m_appWindow->addUserMenuAction( menuName, itemName, item_data);
        }
        // Refresh menu list
        populateMenuTree();

        // Remove scripts elements that have been added to the menu.
        if ( ! m_scriptsTree->selectedItems().isEmpty() )
        {
            remScriptClicked();
        }
    }
}
/**
* Removes item from custom menu, or custom menu itself if selected.
*/
void ManageCustomMenus::remItemClicked()
{
    QTreeWidgetItem* item = getCurrentMenuSelection();
    if ( item == 0 )
    {
        QMessageBox::information(this, "MantidPlot", "No item selected - please select a script or menu in the right-hand list.");
    }
    else
    {
        if ( item->parent() != 0 )
        {
            // Delete menu sub-item
            QTreeWidgetItem* menu = item->parent();
            m_appWindow->removeUserMenuAction(menu->text(0), item->text(0));
        }
        else
        {
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
void ManageCustomMenus::addMenuClicked()
{
    bool ok(false);
    QString name = QInputDialog::getText(this, "Create a Menu", "Menu name:", QLineEdit::Normal, "", &ok);
    if ( ok )
    {
        if( m_menusTree->findItems(name, Qt::MatchFixedString | Qt::MatchCaseSensitive ).isEmpty() )
        {
            if( !name.isEmpty() ) 
            {
                m_appWindow->addUserMenu(name);
                populateMenuTree();
            }
        }
        else
        {
            QMessageBox::information(this, "MantidPlot", "A menu with that name already exists.");
        }
    }
}
/**
* Opens web browser to wiki page for this dialog.
*/
void ManageCustomMenus::helpClicked()
{
    QUrl helpUrl("http://www.mantidproject.org/ManageCustomMenus");
    QDesktopServices::openUrl(helpUrl);
}
