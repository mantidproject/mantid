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
#include "ApplicationWindow.h"
#include "Folder.h"
#include "pixmaps.h"

#include <QApplication>
#include <QDateTime>

Folder::Folder(Folder *parent, const QString &name)
    : QObject(parent),
      birthdate(QDateTime::currentDateTime().toString(Qt::LocalDate)),
      d_log_info(QString()), myFolderListItem(0), d_active_window(0) {
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
    QObject *f;
    foreach (f, folderList)
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
  return NULL;
}

Folder *Folder::findSubfolder(const QString &s, bool caseSensitive,
                              bool partialMatch) {
  QObjectList folderList = children();
  if (!folderList.isEmpty()) {
    QObject *f;
    foreach (f, folderList) {
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
  return 0;
}

MdiSubWindow *Folder::findWindow(const QString &s, bool windowNames,
                                 bool labels, bool caseSensitive,
                                 bool partialMatch) {
  MdiSubWindow *w;

  auto qt_cs = Qt::CaseInsensitive;
  if (!caseSensitive)
    qt_cs = Qt::CaseInsensitive;

  foreach (w, lstWindows) {
    if (windowNames) {
      QString name = w->objectName();

      if (partialMatch && name.startsWith(s, qt_cs))
        return w;
      else if (caseSensitive && name == s)
        return w;
      else {
        QString text = s;
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
        QString text = s;
        if (label == text.toLower())
          return w;
      }
    }
  }
  return 0;
}

MdiSubWindow *Folder::window(const QString &name, const char *cls,
                             bool recursive) {
  foreach (MdiSubWindow *w, lstWindows) {
    if (w->inherits(cls) && name == w->objectName())
      return w;
  }

  if (!recursive)
    return NULL;
  foreach (QObject *f, children()) {
    MdiSubWindow *w = (static_cast<Folder *>(f))->window(name, cls, true);
    if (w)
      return w;
  }
  return NULL;
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
    d_active_window = NULL;

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
    QObject *f;
    foreach (f, folderList)
      size += sizeof(static_cast<Folder *>(f)); // FIXME: Doesn't this function
                                                // add the size of pointers
                                                // together? For what?
  }

  MdiSubWindow *w;
  foreach (w, lstWindows)
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

/**
 * Load the folder structure from a Mantid project file
 *
 * @param lines :: lines containing the folder structure
 * @param app :: the current application window instance
 * @param fileVersion :: the version of the file
 */
void Folder::loadFromProject(const std::string &lines, ApplicationWindow *app, const int fileVersion)
{
    (void)app; //suppress unused variable warnings
    throw std::runtime_error("Not yet implemented.");
}

/**
 * Save the folder structure to a Mantid project file.
 *
 * @param app :: the current application window instance
 * @return string represnetation of the folder's data
 */
std::string Folder::saveToProject(ApplicationWindow *app)
{
    QString text;
    bool isCurrentFolder = app->currentFolder() == this;
    int windowCount = 0;

    text += saveFolderHeader(isCurrentFolder);
    text += saveFolderSubWindows(app, this, windowCount);
    text += saveFolderFooter();

    return text.toStdString();
}

/**
 * Generate the opening tags and meta information about
 * a folder record for the Mantid project file.
 *
 * @param isCurrentFolder :: whether this folder is the current one.
 * @return string representation of the folder's header data
 */
QString Folder::saveFolderHeader(bool isCurrentFolder)
{
    QString text;

    // Write the folder opening tag
    text += "<folder>\t" + QString(objectName()) + "\t" +
            birthDate() + "\t" + modificationDate();

    // label it as current if necessary
    if (isCurrentFolder) {
        text += "\tcurrent";
    }

    text += "\n";
    text += "<open>" + QString::number(folderListItem()->isExpanded()) +
            "</open>\n";
    return text;
}

/**
 * Generate the subfolder and subwindow records for the current folder.
 * This method will recursively convert subfolders to their text representation
 *
 * @param app :: the current application window instance
 * @param folder :: the folder to generate the text for.
 * @param windowCount :: count of the number of windows
 * @return string representation of the folder's subfolders
 */
QString Folder::saveFolderSubWindows(ApplicationWindow* app, Folder * folder,
                                     int &windowCount)
{
    QString text;

    // Write windows
    QList<MdiSubWindow *> windows = folder->windowsList();
    foreach (MdiSubWindow *w, windows) {
      Mantid::IProjectSerialisable *ips =
          dynamic_cast<Mantid::IProjectSerialisable *>(w);

      if (ips) {
        text += QString::fromUtf8(ips->saveToProject(app).c_str());
      }

      ++windowCount;
    }

    // Write subfolders
    QList<Folder *> subfolders = folder->folders();
    foreach (Folder *f, subfolders) { text += saveFolderSubWindows(app, f, windowCount); }

    return text;
}

/**
 * Generate the closing folder data and end tag.
 * @return footer string for this folder
 */
QString Folder::saveFolderFooter()
{
    return "</folder>\n";
}

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
