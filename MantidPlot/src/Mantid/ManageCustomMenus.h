// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANAGE_CUSTOM_MENUS_H
#define MANTID_MANAGE_CUSTOM_MENUS_H

#include "MantidQtWidgets/Common/MantidDialog.h"
#include "ui_ManageCustomMenus.h"
#include <QDialog>

class ApplicationWindow;

/**
This class handles the "Manage Custom Menus" dialog for MantidPlot, in which
users can
add custom scripts or custom Qt interfaces to a menu in MantidPlot.

@author Michael Whitty, ISIS
*/
class ManageCustomMenus : public MantidQt::API::MantidDialog {
  Q_OBJECT
public:
  explicit ManageCustomMenus(
      QWidget *parent = nullptr);

private:
  void initLayout();
  void populateMenuTree();
  QList<QTreeWidgetItem *> getCurrentSelection();
  QTreeWidgetItem *getCurrentMenuSelection();
private slots:
  void addScriptClicked();
  void remScriptClicked();
  void addItemClicked();
  void remItemClicked();
  void addMenuClicked();
  void helpClicked();

private:
  Ui::ManageCustomMenus m_uiForm;
  QMap<QTreeWidgetItem *, QObject *> m_widgetMap;
  QTreeWidget *m_scriptsTree;
  QTreeWidget *m_customInterfacesTree;
  QTreeWidget *m_menusTree;
  ApplicationWindow *m_appWindow;
};

#endif /* MANTID_MANAGE_CUSTOM_MENUS_H */