#ifndef MANTID_MANAGE_CUSTOM_MENUS_H
#define MANTID_MANAGE_CUSTOM_MENUS_H

#include <QDialog>
#include "ui_ManageCustomMenus.h"

class ApplicationWindow;

class ManageCustomMenus : public QDialog, private Ui::ManageCustomMenus
{
	Q_OBJECT
public:
	ManageCustomMenus(QWidget *parent = 0);
private:
    void initLayout();
    void populateMenuTree();
    void getCustomInterfaceList();
    QList<QTreeWidgetItem*> getCurrentSelection();
    QTreeWidgetItem* getCurrentMenuSelection();
private slots:
    void addScriptClicked();
    void remScriptClicked();
    void addItemClicked();
    void remItemClicked();
    void addMenuClicked();
    void helpClicked();

private:
    Ui::ManageCustomMenus m_uiForm;
    QMap<QTreeWidgetItem*,QObject*> m_widgetMap;
    QTreeWidget* m_scriptsTree;
    QTreeWidget* m_customInterfacesTree;
    QTreeWidget* m_menusTree;
    ApplicationWindow* m_appWindow;
};

#endif /* MANTID_MANAGE_CUSTOM_MENUS_H */