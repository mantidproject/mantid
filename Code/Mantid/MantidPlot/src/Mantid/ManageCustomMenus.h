#ifndef MANTID_MANAGE_CUSTOM_MENUS_H
#define MANTID_MANAGE_CUSTOM_MENUS_H

#include <QDialog>
#include "ui_ManageCustomMenus.h"

class ApplicationWindow;

/** 
This class handles the "Manage Custom Menus" dialog for MantidPlot, in which users can
add custom scripts or custom Qt interfaces to a menu in MantidPlot.

@author Michael Whitty, ISIS

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class ManageCustomMenus : public QDialog
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