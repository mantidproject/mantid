/***************************************************************************
    File                 : Folder.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net, knut.franke*gmx.de
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
#include "pixmaps.h"
#include "ApplicationWindow.h"

#include <QApplication>
#include <QDateTime>


Folder::Folder( Folder *parent, const QString &name )
    : QObject(parent), d_log_info(QString()), d_active_window(0)
{
	birthdate = QDateTime::currentDateTime ().toString(Qt::LocalDate);
	setObjectName(name);
}

QList<Folder*> Folder::folders()
{
	QList<Folder*> lst;
	foreach(QObject *f, children())
		lst.append((Folder*) f);
	return lst;
}

QStringList Folder::subfolders()
{
	QStringList list = QStringList();
	QObjectList folderList = children();
	if (!folderList.isEmpty()){
		QObject * f;
		foreach(f,folderList)
			list << static_cast<Folder *>(f)->objectName();
	}
	return list;
}

QString Folder::path()
{
    QString s = "/" + QString(objectName()) + "/";
    Folder *parentFolder = (Folder *)parent();
    while (parentFolder){
        s.prepend("/" + QString(parentFolder->objectName()));
        parentFolder = (Folder *)parentFolder->parent();
	}
    return s;
}

int Folder::depth()
{
	int d = 0;
    Folder *parentFolder = (Folder *)parent();
    while (parentFolder){
        ++d;
        parentFolder = (Folder *)parentFolder->parent();
	}
    return d;
}

Folder* Folder::folderBelow()
{
	QList<Folder*> lst = folders();
	if (!lst.isEmpty())
		return lst.first();

	Folder *parentFolder = (Folder *)parent();
	Folder *childFolder = this;
	while (parentFolder && childFolder){
		lst = parentFolder->folders();
		int pos = lst.indexOf(childFolder) + 1;
		if (pos < lst.size())
			return lst.at(pos);

		childFolder = parentFolder;
		parentFolder = (Folder *)parentFolder->parent();
	}
	return NULL;
}

Folder* Folder::findSubfolder(const QString& s, bool caseSensitive, bool partialMatch)
{
	QObjectList folderList = children();
	if (!folderList.isEmpty()){
		QObject * f;
		foreach(f,folderList){
			QString name = static_cast<Folder *>(f)->objectName();
			if (partialMatch){
				if (caseSensitive && name.startsWith(s,Qt::CaseSensitive))
					return static_cast<Folder *>(f);
				else if (!caseSensitive && name.startsWith(s,Qt::CaseInsensitive))
					return static_cast<Folder *>(f);
			} else {// partialMatch == false
				if (caseSensitive && name == s)
					return static_cast<Folder *>(f);
				else if ( !caseSensitive && (name.toLower() == s.toLower()) )
					return static_cast<Folder *>(f);
			}

			Folder* folder = ((Folder*)f)->findSubfolder(s, caseSensitive, partialMatch);
            if(folder)
                return folder;
		}
	}
	return 0;
}

MdiSubWindow* Folder::findWindow(const QString& s, bool windowNames, bool labels,
							 bool caseSensitive, bool partialMatch)
{
	MdiSubWindow* w;
	foreach(w,lstWindows){
		if (windowNames){
			QString name = w->objectName();
			if (partialMatch && name.startsWith(s, caseSensitive))
				return w;
			else if (caseSensitive && name == s)
				return w;
			else{
				QString text = s;
				if (name == text.lower())
					return w;
			}
		}

		if (labels){
			QString label = w->windowLabel();
			if (partialMatch && label.startsWith(s, caseSensitive))
				return w;
			else if (caseSensitive && label == s)
				return w;
			else{
				QString text = s;
				if (label == text.lower())
					return w;
			}
		}
	}
	return 0;
}

MdiSubWindow *Folder::window(const QString &name, const char *cls, bool recursive)
{
	foreach (MdiSubWindow *w, lstWindows){
		if (w->inherits(cls) && name == w->objectName())
			return w;
	}

	if (!recursive) return NULL;
	foreach (QObject *f, children()){
		MdiSubWindow *w = ((Folder*)f)->window(name, cls, true);
		if (w) return w;
	}
	return NULL;
}

void Folder::addWindow( MdiSubWindow *w )
{
	if (w) {
		lstWindows.append( w );
		w->setFolder(this);
	}
}

void Folder::removeWindow( MdiSubWindow *w )
{
	if (!w)
		return;

	if (d_active_window && d_active_window == w)
		d_active_window = NULL;

	int index = lstWindows.indexOf(w);
	if (index >= 0 && index < lstWindows.size())
		lstWindows.removeAt(index);
}

QString Folder::sizeToString()
{
	int size = 0;

	QObjectList folderList = children();
	if (!folderList.isEmpty()){
		QObject *f;
		foreach(f,folderList)
			size +=  sizeof(static_cast<Folder *>(f)); // FIXME: Doesn't this function add the size of pointers together? For what?
	}

	MdiSubWindow * w;
	foreach(w, lstWindows)
		size += sizeof(w);

	return QString::number(8*size/1024.0,'f',1)+" "+tr("kB")+" ("+QString::number(8*size)+" "+tr("bytes")+")";
}

Folder* Folder::rootFolder()
{
	Folder *i = this;
	while(i->parent())
		i = (Folder*)i->parent();
	return i;
}

/*****************************************************************************
 *
 * Class FolderListItem
 *
 *****************************************************************************/

FolderListItem::FolderListItem( Q3ListView *parent, Folder *f )
    : Q3ListViewItem( parent )
{
    myFolder = f;

    setText(0, f->objectName());
	setOpen(true);
	setActive(true);
	setDragEnabled (true);
	setDropEnabled (true);
}

FolderListItem::FolderListItem( FolderListItem *parent, Folder *f )
    : Q3ListViewItem( parent )
{
    myFolder = f;

    setText(0, f->objectName());
	setOpen(true);
	setActive(true);
}

void FolderListItem::setActive( bool o )
{
    if (o)
		setPixmap(0, getQPixmap("folder_open_xpm") );
    else
		setPixmap(0, getQPixmap("folder_closed_xpm") );

	setSelected(o);
}

bool FolderListItem::isChildOf(FolderListItem *src)
{
	FolderListItem *parent = (FolderListItem *)this->parent();
	while (parent){
	if (parent == src)
		return true;

	parent = (FolderListItem *)parent->parent();
	}
	return false;
}

/*****************************************************************************
 *
 * Class FolderListView
 *
 *****************************************************************************/

FolderListView::FolderListView( QWidget *parent, const char *name )
    : Q3ListView( parent, name ), mousePressed( false )
{
    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );

	if (parent){
		connect(this, SIGNAL(collapsed(Q3ListViewItem *)), (ApplicationWindow *)parent, SLOT(modifiedProject()));
		connect(this, SIGNAL(expanded(Q3ListViewItem *)), (ApplicationWindow *)parent, SLOT(modifiedProject()));
		connect(this, SIGNAL(expanded(Q3ListViewItem *)), this, SLOT(expandedItem(Q3ListViewItem *)));
	}
}

void FolderListView::expandedItem(Q3ListViewItem *item)
{
	Q3ListViewItem *next = item->itemBelow();
	if (next)
		setSelected (next, false);
}

void FolderListView::startDrag()
{
Q3ListViewItem *item = currentItem();
if (!item)
	return;

if (item == firstChild() && item->listView()->rootIsDecorated())
	return;//it's the project folder so we don't want the user to move it

QPoint orig = viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );

QPixmap pix;
if (item->rtti() == FolderListItem::RTTI)
	pix = getQPixmap("folder_closed_xpm");
else
	pix = *item->pixmap (0);

Q3IconDrag *drag = new Q3IconDrag(viewport());
drag->setPixmap(pix, QPoint(pix.width()/2, pix.height()/2 ) );

QList<Q3ListViewItem *> lst;
for (item = firstChild(); item; item = item->itemBelow())
	{
	if (item->isSelected())
		lst.append(item);
	}

emit dragItems(lst);
drag->drag();
}

void FolderListView::contentsDropEvent( QDropEvent *e )
{
Q3ListViewItem *dest = itemAt( contentsToViewport(e->pos()) );
if ( dest && dest->rtti() == FolderListItem::RTTI)
	{
	emit dropItems(dest);
	e->accept();
    }
else
	e->ignore();
}

void FolderListView::keyPressEvent ( QKeyEvent * e )
{
if (isRenaming())
	{
	e->ignore();
	return;
	}

if (currentItem()->rtti() == FolderListItem::RTTI &&
	(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return))
	{
	emit doubleClicked(currentItem());
	e->accept();
	}
else if (e->key() == Qt::Key_F2)
	{
	if (currentItem())
		emit renameItem(currentItem());
	e->accept();
	}
else if(e->key() == Qt::Key_A && e->state() == Qt::ControlModifier)
	{
	selectAll(true);
	e->accept();
	}
else if(e->key() == Qt::Key_F7)
	{
	emit addFolderItem();
	e->accept();
	}
else if(e->key() == Qt::Key_F8)
	{
	emit deleteSelection();
	e->accept();
	}
else
	Q3ListView::keyPressEvent ( e );
}

void FolderListView::contentsMouseDoubleClickEvent( QMouseEvent* e )
{
	if (isRenaming())
		{
		e->ignore();
		return;
		}

	Q3ListView::contentsMouseDoubleClickEvent( e );
}

void FolderListView::contentsMousePressEvent( QMouseEvent* e )
{
	Q3ListView::contentsMousePressEvent(e);
	QPoint p( contentsToViewport( e->pos() ) );
	Q3ListViewItem *i = itemAt( p );
	if ( i )
		{// if the user clicked into the root decoration of the item, don't try to start a drag!
		if ( p.x() > header()->cellPos( header()->mapToActual( 0 ) ) +
			treeStepSize() * ( i->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin() ||
			p.x() < header()->cellPos( header()->mapToActual( 0 ) ) )
			{
			presspos = e->pos();
	    	mousePressed = true;
			}
    	}
}

void FolderListView::contentsMouseMoveEvent( QMouseEvent* e )
{
if ( mousePressed && ( presspos - e->pos() ).manhattanLength() > QApplication::startDragDistance() )
	{
	mousePressed = false;
	Q3ListViewItem *item = itemAt( contentsToViewport(presspos) );
	if ( item )
		startDrag();
    }
}

void FolderListView::adjustColumns()
{
for (int i=0; i < columns (); i++)
	adjustColumn(i);
}

/*****************************************************************************
 *
 * Class WindowListItem
 *
 *****************************************************************************/

WindowListItem::WindowListItem( Q3ListView *parent, MdiSubWindow *w )
    : Q3ListViewItem( parent )
{
    myWindow = w;

	setDragEnabled ( true );
}
