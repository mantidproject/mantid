/***************************************************************************
    File                 : MdiSubWindow.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : MDI sub window

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
#include "MdiSubWindow.h"
#include "ApplicationWindow.h"
#include "FloatingWindow.h"
#include "Folder.h"

#include "MantidQtWidgets/Common/IProjectSerialisable.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QEvent>
#include <QMdiArea>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>

#include <fstream>
#include <string>

using std::ifstream;
using std::string;
using namespace Mantid;

MdiSubWindow::MdiSubWindow(QWidget *parent, const QString &label,
                           const QString &name, Qt::WFlags f)
    : MdiSubWindowParent_t(parent, f),
      d_app(static_cast<ApplicationWindow *>(parent)),
      d_folder(d_app->currentFolder()), d_label(label), d_status(Normal),
      d_caption_policy(Both), d_confirm_close(true),
      d_birthdate(QDateTime::currentDateTime().toString(Qt::LocalDate)),
      d_min_restore_size(QSize()) {
  init(parent, label, name, f);
}

MdiSubWindow::MdiSubWindow()
    : MdiSubWindowParent_t(nullptr, nullptr), d_app(nullptr), d_folder(nullptr),
      d_label(""), d_status(Normal), d_caption_policy(Both),
      d_confirm_close(true),
      d_birthdate(QDateTime::currentDateTime().toString(Qt::LocalDate)),
      d_min_restore_size(QSize()) {}

void MdiSubWindow::init(QWidget *parent, const QString &label,
                        const QString &name, Qt::WFlags flags) {
  setParent(parent);
  setObjectName(name);
  setName(name);
  setAttribute(Qt::WA_DeleteOnClose);
  setLocale(parent->locale());
  setWindowFlags(flags);

  d_app = static_cast<ApplicationWindow *>(parent);
  d_folder = d_app->currentFolder();
  d_label = label;

  confirmClose(false);
  if (parent->metaObject()->indexOfSlot(
          QMetaObject::normalizedSignature("changeToDocked(MdiSubWindow*)"))) {
    connect(this, SIGNAL(dockToMDIArea(MdiSubWindow *)), parent,
            SLOT(changeToDocked(MdiSubWindow *)));
    connect(this, SIGNAL(undockFromMDIArea(MdiSubWindow *)), parent,
            SLOT(changeToFloating(MdiSubWindow *)));
  }
}

void MdiSubWindow::updateCaption() {
  switch (d_caption_policy) {
  case Name:
    setWindowTitle(objectName());
    break;

  case Label:
    if (!d_label.isEmpty())
      setWindowTitle(d_label);
    else
      setWindowTitle(objectName());
    break;

  case Both:
    if (!d_label.isEmpty())
      setWindowTitle(objectName() + " - " + d_label);
    else
      setWindowTitle(objectName());
    break;
  }

  QWidget *wrapper = this->getWrapperWindow();
  if (wrapper) {
    wrapper->setWindowTitle(windowTitle());
  }
  emit captionChanged(objectName(), d_label);
}

void MdiSubWindow::setLabel(const QString &label) { d_label = label; }

MantidQt::API::IProjectSerialisable *
MdiSubWindow::loadFromProject(const std::string &lines, ApplicationWindow *app,
                              const int fileVersion) {
  Q_UNUSED(lines);
  Q_UNUSED(app);
  Q_UNUSED(fileVersion);
  throw std::runtime_error(
      "LoadToProject not implemented for raw MdiSubWindow");
}

std::string MdiSubWindow::saveToProject(ApplicationWindow *app) {
  Q_UNUSED(app);
  // By default this is unimplemented and so should return nothing
  return "";
}

std::vector<std::string> MdiSubWindow::getWorkspaceNames() { return {}; }

std::string MdiSubWindow::getWindowName() { return objectName().toStdString(); }

std::string MdiSubWindow::getWindowType() { return metaObject()->className(); }

void MdiSubWindow::resizeEvent(QResizeEvent *e) {
  emit resizedWindow(this);
  MdiSubWindowParent_t::resizeEvent(e);
}

/**
 * Set whether a dialog should be raised when closing the
 * window. If not sets the delete on close flag
 */
void MdiSubWindow::confirmClose(bool ask) {
  d_confirm_close = ask;
  setAttribute(Qt::WA_DeleteOnClose, !ask);
}

/**
 * Override the QWidget's show() slot to show the wrapper window instead.
 */
void MdiSubWindow::show() { setNormal(); }

/**
 * Focus on the window
 */
void MdiSubWindow::setFocus() { QWidget::setFocus(Qt::OtherFocusReason); }

/**
 * Override the QWidget's hide() slot to hide the wrapper window instead.
 */
void MdiSubWindow::hide() { setHidden(); }

/**
 * Override the QWidget's close() slot to close the wrapper window as well.
 */
bool MdiSubWindow::close() {
  QWidget *pw = getWrapperWindow();
  if (!pw) {
    return MdiSubWindowParent_t::close();
  }
  return pw->close();
}

/**
 * Override the QWidget's move(x,y) method to move the wrapper window instead.
 */
void MdiSubWindow::move(int x, int y) {
  QWidget *pw = getWrapperWindow();
  if (pw) {
    QPoint pos = mapTo(pw, QPoint(x, y));
    pw->move(pos);
  }
}

/**
 * Override the QWidget's move(pos) method to move the wrapper window instead.
 */
void MdiSubWindow::move(const QPoint &pos) {
  QWidget *pw = getWrapperWindow();
  if (pw) {
    QPoint pos1 = mapTo(pw, pos);
    pw->move(pos1);
  }
}

/**
 * Resize the window to it's default size
 */
void MdiSubWindow::resizeToDefault() { resize(500, 400); }

/**
 */
void MdiSubWindow::undock() {
  if (!isFloating())
    emit undockFromMDIArea(this);
}

/**
 * @return True if the subwindow is undocked
 */
bool MdiSubWindow::isFloating() const {
  return (this->getFloatingWindow() != nullptr);
}

/**
 */
void MdiSubWindow::dock() {
  if (!isDocked())
    emit dockToMDIArea(this);
}

/**
 * @return True if the subwindow is docked to the MDI area
 */
bool MdiSubWindow::isDocked() const {
  return (this->getDockedWindow() != nullptr);
}

/**
 */
void MdiSubWindow::detach() { emit detachFromParent(this); }

/**
 * Handle the close event.
 * @param e :: A QCloseEvent event.
 */
void MdiSubWindow::closeEvent(QCloseEvent *e) {
  // Default result = do close.
  int result = 0;

  // If you need to confirm the close, ask the user
  if (d_confirm_close) {
    result =
        QMessageBox::information(this, tr("MantidPlot"),
                                 tr("Do you want to hide or delete") +
                                     "<p><b>'" + objectName() + "'</b> ?",
                                 tr("Delete"), tr("Hide"), tr("Cancel"), 0, 2);
  }

  switch (result) {
  case 0: // close
    if (!widget() || widget()->close()) {
      e->accept();
      emit closedWindow(this);
      // Continue; the mdi window should close (?)
    } else {
      QMessageBox::critical(parentWidget(), "MantidPlot - Error",
                            "Window cannot be closed");
      e->ignore();
    }
    break;

  case 1: // hide
    e->ignore();
    emit hiddenWindow(this);
    break;

  case 2: // cancel
    e->ignore();
    break;
  }
}

QString MdiSubWindow::aspect() {
  QString s = tr("Normal");
  switch (d_status) {
  case Normal:
    break;

  case Minimized:
    return tr("Minimized");
    break;

  case Maximized:
    return tr("Maximized");
    break;

  case Hidden:
    return tr("Hidden");
    break;
  }
  return s;
}

QString MdiSubWindow::sizeToString() {
  return QString::number(8. * static_cast<float>(sizeof(this)) / 1024.0, 'f',
                         1) +
         " " + tr("kB");
}

void MdiSubWindow::changeEvent(QEvent *event) {
  if (!isHidden() && event->type() == QEvent::WindowStateChange) {
    Status oldStatus = d_status;
    Status newStatus = Normal;
    if (windowState() & Qt::WindowMinimized) {
      if (oldStatus != Minimized)
        d_min_restore_size = frameSize();
      newStatus = Minimized;
    } else if (windowState() & Qt::WindowMaximized)
      newStatus = Maximized;

    if (newStatus != oldStatus) {
      d_status = newStatus;
      emit statusChanged(this);
    }
  }
  MdiSubWindowParent_t::changeEvent(event);
}

bool MdiSubWindow::eventFilter(QObject *object, QEvent *e) {
  if (e->type() == QEvent::ContextMenu && object == widget()) {
    emit showContextMenu();
    return true;
  }

  // if (e->type() == QEvent::Move && object == widget()){
  //  QObjectList lst = children();
  //  foreach(QObject *o, lst){
  //    if (o->isA("QMenu") && d_app){
  //         d_app->customWindowTitleBarMenu(this, dynamic_cast<QMenu *>(o));
  //      break;
  //    }
  //  }
  //}
  return MdiSubWindowParent_t::eventFilter(object, e);
}

void MdiSubWindow::setStatus(Status s) {
  if (d_status == s)
    return;

  d_status = s;
  emit statusChanged(this);
}

void MdiSubWindow::setHidden() {
  QWidget *pw = getWrapperWindow();
  if (!pw) {
    MdiSubWindowParent_t::hide();
  } else {
    pw->hide();
  }
  setStatus(Hidden);
}

void MdiSubWindow::setNormal() {
  QWidget *wrapper = getWrapperWindow();
  if (wrapper) {
    wrapper->showNormal();
  } else {
    showNormal();
  }
  setStatus(Normal);
}

void MdiSubWindow::setMinimized() {
  setStatus(Minimized);
  QWidget *wrapper = getWrapperWindow();
  if (wrapper) {
    wrapper->showMinimized();
    // d_app->activateNewWindow();
  } else {
    showMinimized();
  }
}

void MdiSubWindow::setMaximized() {
  QWidget *wrapper = getWrapperWindow();
  if (wrapper) {
    wrapper->showMaximized();
  } else {
    showMaximized();
  }
  setStatus(Maximized);
}

QString MdiSubWindow::parseAsciiFile(const QString &fname,
                                     const QString &commentString, int endLine,
                                     int ignoreFirstLines, int maxRows,
                                     int &rows) {
  if (endLine == 2)
    return parseMacAsciiFile(fname, commentString, ignoreFirstLines, maxRows,
                             rows);

  // QTextStream replaces '\r\n' with '\n', therefore we don't need a special
  // treatement in this case!

  QFile f(fname);
  if (!f.open(QIODevice::ReadOnly))
    return QString::null;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  QTextStream t(&f);

  QTemporaryFile tempFile;
  tempFile.open();
  QTextStream temp(&tempFile);

  for (int i = 0; i < ignoreFirstLines;
       i++) // skip first 'ignoreFirstLines' lines
    t.readLine();

  bool validCommentString = !commentString.isEmpty();
  rows = 0;
  if (maxRows <= 0) {    // read all valid lines
    while (!t.atEnd()) { // count the number of valid rows
      QString s = t.readLine();
      if (validCommentString && s.startsWith(commentString))
        continue;

      rows++;
      temp << s + "\n";
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  } else { // we write only 'maxRows' valid rows to the temp file
    while (!t.atEnd() && rows < maxRows) {
      QString s = t.readLine();
      if (validCommentString && s.startsWith(commentString))
        continue;

      rows++;
      temp << s + "\n";
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }
  f.close();

  tempFile.setAutoRemove(false);
  QString path = tempFile.fileName();
  tempFile.close();

  QApplication::restoreOverrideCursor();
  return path;
}

QString MdiSubWindow::parseMacAsciiFile(const QString &fname,
                                        const QString &commentString,
                                        int ignoreFirstLines, int maxRows,
                                        int &rows) {
  ifstream f;
  f.open(fname.toAscii());
  if (!f)
    return QString::null;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QTemporaryFile tempFile;
  tempFile.open();
  QTextStream temp(&tempFile);

  for (int i = 0; i < ignoreFirstLines;
       i++) { // skip first 'ignoreFirstLines' lines
    string s;
    getline(f, s, '\r');
  }

  bool validCommentString = !commentString.isEmpty();
  string comment = commentString.toAscii().constData();
  int commentLength = static_cast<int>(comment.size());
  rows = 0;
  if (maxRows <= 0) {              // read all valid lines
    while (f.good() && !f.eof()) { // count the number of valid rows
      string s;
      getline(f, s, '\r');
      if (validCommentString && s.compare(0, commentLength, comment) == 0)
        continue;

      rows++;
      temp << QString(s.c_str()) + "\n";
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  } else { // we write only 'maxRows' valid rows to the temp file
    while (f.good() && !f.eof() && rows < maxRows) {
      string s;
      getline(f, s, '\r');
      if (validCommentString && s.compare(0, commentLength, comment) == 0)
        continue;

      rows++;
      temp << QString(s.c_str()) + "\n";
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }
  f.close();

  tempFile.setAutoRemove(false);
  QString path = tempFile.fileName();
  tempFile.close();

  QApplication::restoreOverrideCursor();
  return path;
}

/**
 * Return a pointer to the wrapper window if it is a FloatingWindow or NULL
 * otherwise.
 */
FloatingWindow *MdiSubWindow::getFloatingWindow() const {
  if (!parent() || !parent()->parent())
    return nullptr;
  return dynamic_cast<FloatingWindow *>(parent()->parent());
}

/**
 * Return a pointer to the wrapper window if it is a QMdiSubWindow or NULL
 * otherwise.
 */
QMdiSubWindow *MdiSubWindow::getDockedWindow() const {
  if (!parent())
    return nullptr;
  return dynamic_cast<QMdiSubWindow *>(parent());
}

/**
 * Return a pointer to the wrapper window.
 */
QWidget *MdiSubWindow::getWrapperWindow() const {
  QWidget *wrapper = getDockedWindow();
  if (!wrapper) {
    wrapper = getFloatingWindow();
  }
  return wrapper;
}

/**
 * Wrapper for the resize method.
 */
void MdiSubWindow::resize(int w, int h) {
  QWidget *pw = getWrapperWindow();
  if (pw) {
    setGeometry(x(), y(), w, h);
    pw->adjustSize();
  } else {
    QFrame::resize(w, h);
  }
}

/**
 * Wrapper for the resize method.
 */
void MdiSubWindow::resize(const QSize &size) {
  QWidget *pw = getWrapperWindow();
  if (pw) {
    setGeometry(pw->x(), pw->y(), size.width(), size.height());
    pw->adjustSize();
  } else {
    QFrame::resize(size);
  }
}

QSize MdiSubWindow::sizeHint() const { return size(); }
