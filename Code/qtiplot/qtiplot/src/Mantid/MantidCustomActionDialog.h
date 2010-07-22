#ifndef MANTIDCUSTOMACTIONDIALOG_H_
#define MANTIDCUSTOMACTIONDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include <QDialog>
#include <QPoint>
#include <QTreeWidget>
#include <QMap>

//----------------------------------
// Forward declarations
//----------------------------------
class ApplicationWindow;
class ActionTreeWidget;
class QMouseEvent;

/** 
    This class is a replacement for the Qtiplot CustomActionDialog class as there were a lot
    of changes to be made for us to be able to use it as we wanted.

    @author Martyn Gigg, Tessella Support Services plc
    @date 18/12/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class MantidCustomActionDialog : public QDialog
{
  //Qt macro for signal/slot mechanism
  Q_OBJECT
  
  public:
  
  //Constructor
  MantidCustomActionDialog(QWidget* parent, Qt::WFlags flags = 0);

protected slots:
  
  //Catch a context menu request
  void popupMenu(const QPoint & pos);

private:
  MantidCustomActionDialog(const MantidCustomActionDialog&);
  MantidCustomActionDialog& operator=(const MantidCustomActionDialog&);
  
  //Initialize the object
  void init();

  ///Import scripts into the selected menu (does the actual work)
  void importItems(const QList<QTreeWidgetItem*> & custom_items, bool remove);
  
private slots:

  //Choose file(s)
  void addFileClicked();
 //Remove from file(s) from list
  void removeFileClicked();
  // Add a menu
  void addMenuClicked();
  // Remove a menu
  void removeMenuClicked();
  //(Re)-populate the tree of scripts based on the current layout of the map stored in the ApplicationWindow
  //object
  void refreshMenuTree();
  /// Add script items to the file tree
  void addFileItems(const QStringList& fileList);

  /// Import from the script file tree
  void importFromFileTree();

  /// Import from the custom window tree
  void importFromCustomTree();

  /// Import all selections
  void importAllSelected();

  ///Handle a text change
  void itemTextChanged(QTreeWidgetItem*);

private:
  //A tree widget displaying the state of the current script menus
  ActionTreeWidget *m_menuTree;

  //A tree view displaying a list of scripts to add to the selected menu
  ActionTreeWidget *m_fileTree;

  //A tree widget displaying a list of available customised user interfaces
  ActionTreeWidget *m_customUITree;

  ///A map of model indices to widgets
  QMap<QTreeWidgetItem*,QObject*> m_widgetMap;
  
   //Pointer to the application window
  ApplicationWindow* m_appWindow;
  
  //The last directory browsed
  QString m_lastDirectory;
};


/**
 * A small, more specialized version of a QTreeWidget that I can use to override
 * a virtual function
 */
class ActionTreeWidget : public QTreeWidget
{
  Q_OBJECT
  
public:
  ///Default constructor
  ActionTreeWidget(QWidget *parent = 0);
  
signals:
  ///New data for an item
  void textChange(QTreeWidgetItem*);

protected slots:

  /// Data is edited
  void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight);

protected:
  ///Mouse press event
  void mousePressEvent(QMouseEvent* event);
};

#endif //MANTIDCUSTOMACTIONDIALOG_H_
