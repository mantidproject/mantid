/***************************************************************************
    File                 : Folder.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net,
 knut.franke*gmx.de
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
#include "Folder.h"
#include "ApplicationWindow.h"
#include "MantidQtWidgets/Common/pixmaps.h"
#include <QApplication>
#include <QDateTime>

using namespace MantidQt::API;

Folder::Folder(Folder *parent, const QString &name)
    : QObject(parent),
      birthdate(QDateTime::currentDateTime().toString(Qt::LocalDate)),
      d_log_info(QString()), myFolderListItem(nullptr),
      d_active_window(nullptr) {
  setObjectName(name);
}

QList<Folder *> Folder::folders() {
  QList<Folder *> lst;
  foreach (QObject *f, children())
    lst.append(static_cast<Folder *>(f));
  return lst;
}

QStringList Folder::subfolders() {
  QStringList list;
  QObjectList folderList = children();
  if (!folderList.isEmpty()) {
    foreach (QObject *f, folderList)
      list << static_cast<Folder *>(f)->objectName();
  }
  return list;
}

QString Folder::path() {
  QString s = "/" + QString(objectName()) + "/";
  Folder *parentFolder = static_cast<Folder *>(parent());
  while (parentFolder) {
    s.prepend("/" + QString(parentFolder->objectName()));
    parentFolder = static_cast<Folder *>(parentFolder->parent());
  }
  return s;
}

int Folder::depth() {
  int d = 0;
  Folder *parentFolder = static_cast<Folder *>(parent());
  while (parentFolder) {
    ++d;
    parentFolder = static_cast<Folder *>(parentFolder->parent());
  }
  return d;
}

Folder *Folder::folderBelow() {
  QList<Folder *> lst = folders();
  if (!lst.isEmpty())
    return lst.first();

  Folder *parentFolder = static_cast<Folder *>(parent());
  Folder *childFolder = this;
  while (parentFolder && childFolder) {
    lst = parentFolder->folders();
    int pos = lst.indexOf(childFolder) + 1;
    if (pos < lst.size())
      return lst.at(pos);

    childFolder = parentFolder;
    parentFolder = static_cast<Folder *>(parentFolder->parent());
  }
  return nullptr;
}

Folder *Folder::findSubfolder(const QString &s, bool caseSensitive,
                              bool partialMatch) {
  QObjectList folderList = children();
  if (!folderList.isEmpty()) {
    foreach (QObject *f, folderList) {
      QString name = static_cast<Folder *>(f)->objectName();
      if (partialMatch) {
        if (caseSensitive && name.startsWith(s, Qt::CaseSensitive))
          return static_cast<Folder *>(f);
        else if (!caseSensitive && name.startsWith(s, Qt::CaseInsensitive))
          return static_cast<Folder *>(f);
      } else { // partialMatch == false
        if (caseSensitive && name == s)
          return static_cast<Folder *>(f);
        else if (!caseSensitive && (name.toLower() == s.toLower()))
          return static_cast<Folder *>(f);
      }

      Folder *folder = (static_cast<Folder *>(f))
                           ->findSubfolder(s, caseSensitive, partialMatch);
      if (folder)
        return folder;
    }
  }
  return nullptr;
}

MdiSubWindow *Folder::findWindow(const QString &s, bool windowNames,
                                 bool labels, bool caseSensitive,
                                 bool partialMatch) {
  auto qt_cs = Qt::CaseInsensitive;
  if (!caseSensitive)
    qt_cs = Qt::CaseInsensitive;

  foreach (MdiSubWindow *w, lstWindows) {
    if (windowNames) {
      QString name = w->objectName();

      if (partialMatch && name.startsWith(s, qt_cs))
        return w;
      else if (caseSensitive && name == s)
        return w;
      else {
        const QString &text = s;
        if (name == text.toLower())
          return w;
      }
    }

    if (labels) {
      QString label = w->windowLabel();
      if (partialMatch && label.startsWith(s, qt_cs))
        return w;
      else if (caseSensitive && label == s)
        return w;
      else {
        const QString &text = s;
        if (label == text.toLower())
          return w;
      }
    }
  }
  return nullptr;
}

MdiSubWindow *Folder::window(const QString &name, const char *cls,
                             bool recursive) {
  foreach (MdiSubWindow *w, lstWindows) {
    if (w->inherits(cls) && name == w->objectName())
      return w;
  }

  if (!recursive)
    return nullptr;
  foreach (QObject *f, children()) {
    MdiSubWindow *w = (static_cast<Folder *>(f))->window(name, cls, true);
    if (w)
      return w;
  }
  return nullptr;
}

void Folder::addWindow(MdiSubWindow *w) {
  if (w) {
    lstWindows.append(w);
    connect(w, SIGNAL(closedWindow(MdiSubWindow *)), this,
            SLOT(removeWindow(MdiSubWindow *)));
  }
}

void Folder::removeWindow(MdiSubWindow *w) {
  if (!w)
    return;

  if (d_active_window && d_active_window == w)
    d_active_window = nullptr;

  int index = lstWindows.indexOf(w);
  if (index >= 0 && index < lstWindows.size())
    lstWindows.removeAt(index);
}

/**
 * Returns true if this folder contains a sub-window.
 * @param w :: A sub-window to check.
 */
bool Folder::hasWindow(MdiSubWindow *w) const { return lstWindows.contains(w); }

QString Folder::sizeToString() {
  size_t size = 0;

  QObjectList folderList = children();
  if (!folderList.isEmpty()) {
    foreach (QObject *f, folderList)
      size += sizeof(static_cast<Folder *>(f)); // FIXME: Doesn't this function
                                                // add the size of pointers
                                                // together? For what?
  }

  foreach (MdiSubWindow *w, lstWindows)
    size += sizeof(w);

  return QString::number(double(8 * size) / 1024.0, 'f', 1) + " " + tr("kB") +
         " (" + QString::number(8 * size) + " " + tr("bytes") + ")";
}

Folder *Folder::rootFolder() {
  Folder *i = this;
  while (i->parent())
    i = static_cast<Folder *>(i->parent());
  return i;
}

bool Folder::isEmpty() const { return lstWindows.isEmpty(); }

/*****************************************************************************
 *
 * Class FolderListItem
 *
 *****************************************************************************/

FolderListItem::FolderListItem(QTreeWidget *parent, Folder *f)
    : QTreeWidgetItem(parent) {
  myFolder = f;

  setText(0, f->objectName());
  setExpanded(true);
  setActive(true);
  setFlags(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
}

FolderListItem::FolderListItem(FolderListItem *parent, Folder *f)
    : QTreeWidgetItem(parent) {
  myFolder = f;

  setText(0, f->objectName());
  setExpanded(true);
  setActive(true);
}

void FolderListItem::setActive(bool o) {
  if (o) {
    setIcon(0, getQPixmap("folder_open_xpm"));
  } else {
    setIcon(0, getQPixmap("folder_closed_xpm"));
  }
  setSelected(o);
}

bool FolderListItem::isChildOf(FolderListItem *src) {
  FolderListItem *parent = dynamic_cast<FolderListItem *>(this->parent());
  while (parent) {
    if (parent == src)
      return true;

    parent = dynamic_cast<FolderListItem *>(parent->parent());
  }
  return false;
}

/*****************************************************************************
 *
 * Class FolderListView
 *
 *****************************************************************************/

FolderListView::FolderListView(QWidget *parent, const char *name)
    : QTreeWidget(parent) {
  setWindowTitle(name);
  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);

  if (parent) {
    connect(this, SIGNAL(collapsed(const QModelIndex &)),
            dynamic_cast<ApplicationWindow *>(parent), SLOT(modifiedProject()));
    connect(this, SIGNAL(expanded(const QModelIndex &)),
            dynamic_cast<ApplicationWindow *>(parent), SLOT(modifiedProject()));
    connect(this, SIGNAL(expanded(const QModelIndex &)), this,
            SLOT(expandedItem(const QModelIndex &)));
  }
}

void FolderListView::expandedItem(const QModelIndex &index) {
  auto item = itemFromIndex(index);
  QTreeWidgetItem *next = itemBelow(item);
  if (next) {
    next->setSelected(true);
  }
}

void FolderListView::adjustColumns() {
  for (int i = 0; i < columnCount(); i++) {
    resizeColumnToContents(i);
  }
}

QTreeWidgetItem *FolderListView::firstChild() { return topLevelItem(0); }

/*****************************************************************************
 *
 * Class WindowListItem
 *
 *****************************************************************************/

WindowListItem::WindowListItem(QTreeWidget *parent, MdiSubWindow *w)
    : QTreeWidgetItem(parent) {
  myWindow = w;
}
