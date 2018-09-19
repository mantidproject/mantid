/***************************************************************************
    File                 : Folder.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Folder for the project explorer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef FOLDER_H
#define FOLDER_H

#include <QEvent>
#include <QObject>
#include <QTreeWidget>

#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MdiSubWindow.h"

class FolderListItem;
class Table;
class Matrix;
class MultiLayer;
class Note;

//! Folder for the project explorer
class Folder : public QObject {
  Q_OBJECT

public:
  Folder(Folder *parent, const QString &name);

  QString name() const { return this->objectName(); }
  QList<MdiSubWindow *> windowsList() { return lstWindows; }

  void addWindow(MdiSubWindow *w);
  bool hasWindow(MdiSubWindow *w) const;

  //! The list of subfolder names, including first generation children only
  QStringList subfolders();

  //! The list of subfolders
  QList<Folder *> folders();

  //! Pointer to the subfolder called s
  Folder *findSubfolder(const QString &s, bool caseSensitive = true,
                        bool partialMatch = false);

  //! Pointer to the first window matching the search criteria
  MdiSubWindow *findWindow(const QString &s, bool windowNames, bool labels,
                           bool caseSensitive, bool partialMatch);

  //! get a window by name
  /**
   * Returns the first window with given name that inherits class cls;
   * NULL on failure. If recursive is true, do a depth-first recursive
   * search.
   */
  MdiSubWindow *window(const QString &name, const char *cls = "MdiSubWindow",
                       bool recursive = false);
  //! Return table named name or NULL
  Table *table(const QString &name, bool recursive = false) {
    return reinterpret_cast<Table *>(window(name, "Table", recursive));
  }
  //! Return matrix named name or NULL
  Matrix *matrix(const QString &name, bool recursive = false) {
    return reinterpret_cast<Matrix *>(window(name, "Matrix", recursive));
  }
  //! Return graph named name or NULL
  MultiLayer *graph(const QString &name, bool recursive = false) {
    return reinterpret_cast<MultiLayer *>(
        window(name, "MultiLayer", recursive));
  }
  //! Return note named name or NULL
  Note *note(const QString &name, bool recursive = false) {
    return reinterpret_cast<Note *>(window(name, "Note", recursive));
  }

  //! The complete path of the folder in the project tree
  QString path();

  //! The depth of the folder in the project tree
  int depth();

  Folder *folderBelow();

  //! The root of the hierarchy this folder belongs to.
  Folder *rootFolder();

  //! Size of the folder as a string
  QString sizeToString();

  QString birthDate() { return birthdate; };
  void setBirthDate(const QString &s) { birthdate = s; };

  QString modificationDate() { return modifDate; };
  void setModificationDate(const QString &s) { modifDate = s; };

  //! Pointer to the corresponding QListViewItem in the main application
  FolderListItem *folderListItem() { return myFolderListItem; };
  void setFolderListItem(FolderListItem *it) { myFolderListItem = it; };

  MdiSubWindow *activeWindow() { return d_active_window; };
  void setActiveWindow(MdiSubWindow *w) { d_active_window = w; };

  QString logInfo() { return d_log_info; };
  void clearLogInfo() { d_log_info = QString(); };

  bool isEmpty() const;

private:
  /// Save header information about the folder
  QString saveFolderHeader(bool isCurrentFolder);
  /// Recursively save subwindows and subfolders
  QString saveFolderSubWindows(ApplicationWindow *app, Folder *,
                               int &windowCount);
  /// Save footer infromation about the folder
  QString saveFolderFooter();

public slots:
  /// Mantid: made this a slot for use with script messages when there is no
  /// script window
  void appendLogInfo(const QString &text) { d_log_info += text; };
  void removeWindow(MdiSubWindow *w);

protected:
  QString birthdate, modifDate;
  QString d_log_info;
  QList<MdiSubWindow *> lstWindows;
  FolderListItem *myFolderListItem;

  //! Pointer to the active window in the folder
  MdiSubWindow *d_active_window;
};

/*****************************************************************************
 *
 * Class WindowListItem
 *
 *****************************************************************************/
//! Windows list item class
class WindowListItem : public QTreeWidgetItem {
public:
  WindowListItem(QTreeWidget *parent, MdiSubWindow *w);

  MdiSubWindow *window() { return myWindow; };

protected:
  MdiSubWindow *myWindow;
};

/*****************************************************************************
 *
 * Class FolderListItem
 *
 *****************************************************************************/
//! Folders list item class
class FolderListItem : public QTreeWidgetItem {
public:
  FolderListItem(QTreeWidget *parent, Folder *f);
  FolderListItem(FolderListItem *parent, Folder *f);

  enum { RTTI = 1001 };

  void setActive(bool o);
  void cancelRename(int) { return; };
  int rtti() const { return (int)RTTI; };

  Folder *folder() { return myFolder; };

  //! Checks weather the folder item is a grandchild of the source folder
  /**
   * @param src :: source folder item
   */
  bool isChildOf(FolderListItem *src);

protected:
  Folder *myFolder;
};

/*****************************************************************************
 *
 * Class FolderListView
 *
 *****************************************************************************/
//! Folder list view class
class FolderListView : public QTreeWidget {
  Q_OBJECT

public:
  FolderListView(QWidget *parent = nullptr, const char *name = nullptr);
  QTreeWidgetItem *firstChild();

public slots:
  void adjustColumns();

protected slots:
  void expandedItem(const QModelIndex &index);

signals:
  void deleteSelection();
};

#endif
