#ifndef MANTIDCUSTOMACTIONDIALOG_H_
#define MANTIDCUSTOMACTIONDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include <QDialog>
#include <QPoint>

//----------------------------------
// Forward declarations
//----------------------------------
class ApplicationWindow;
class QTreeWidget;
class QPushButton;
class QLineEdit;
class QComboBox;

/** 
    This class is a replacement for the Qtiplot CustomActionDialog class as there were a lot
    of changes to be made for us to be able to use it as we wanted.

    @author Martyn Gigg, Tessella Support Services plc
    @date 18/12/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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
  
private slots:

    //Adds scripts to the appropriate menus
  void addActions();
  //Adds a script to the list and updates menus accordingly
  void addAction(const QString & menuText);
  //Removes the script menu item
  void removeSelectedItem();
  //Choose a file
  void chooseFile();
  //Choose a file
  void chooseDirectory();
  //Add a new menu when requested
  void handleComboSelection(const QString &);
  //Validate user input
  bool validUserInput();
  //(Re)-populate the tree of scripts based on the current layout of the map stored in the ApplicationWindow
  //object
  void refreshScriptTree();

private:
  //A tree widget showing the script layout
  QTreeWidget *m_tree;
  
  //Text fields
  QLineEdit *fileBox, *dirBox;
  QPushButton *fileBtn, *dirBtn;

  //Buttons
  QPushButton *buttonAdd, *buttonRemove, *buttonCancel;

  //Combo menu box
  QComboBox *menuList;

  //Pointer to the application window
  ApplicationWindow* m_appWindow;
};

#endif //MANTIDCUSTOMACTIONDIALOG_H_
