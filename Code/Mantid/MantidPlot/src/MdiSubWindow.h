/***************************************************************************
    File                 : MdiSubWindow.h
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
#ifndef MdiSubWindow_H
#define MdiSubWindow_H

#include <QMdiSubWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QMainWindow>
#include <QFrame>

#include <stdexcept>

class QEvent;
class QCloseEvent;
class QString;
class FloatingWindow;
class ApplicationWindow;

class MdiSubWindowParent_t: public QFrame
{
  Q_OBJECT
public:
  MdiSubWindowParent_t(QWidget* parent, Qt::WFlags f = 0):
    QFrame(parent,f),
    m_widget(NULL)
  {}
  void setWidget(QWidget* w)
  {
    if (w == NULL)
      {// removing widget
        if (m_widget)
        {
          layout()->takeAt(0);
        }
        m_widget = NULL;
        return;
      }

    // widget cannot be replaced
    if (m_widget)
    {
      throw std::runtime_error("Widget already set");
    }

    // setting the internal widget
    if ( this->layout() == NULL )
    {
      QVBoxLayout* layout = new QVBoxLayout(this);
      layout->setContentsMargins(0,0,0,0);
      layout->addWidget(w);
    }
    else
    {
      layout()->addWidget(w);
    }
    m_widget = w;
    m_widget->setParent(this); // I am not sure about this
  }
  ~MdiSubWindowParent_t()
  {
    //std::cerr << "MdiSubWindowParent_t deleted\n";
  }
  QWidget* widget() {return m_widget;}
  const QWidget* widget() const {return m_widget;}
protected:
  QWidget* m_widget;
};

/**
 * \brief Base class of all MDI client windows.
 *
 * These are the main objects of every Qtiplot project.
 * All content (apart from the directory structure) is managed by subclasses of MdiSubWindow.
 *
 * With introduction of floating windows this class is no longer a sub-window (with window title and system menu)
 * but rather the internal widget of a QMdiSubWindow or a FloatingWindow. The outer window can be changed between
 * FloatingWindow and QMdiSubWindow at runtime using ApplicationWindow::changeToFloating(...) and
 * ApplicationWindow::changeToDocked(...) methods. MdiSubWindow overrides show(), hide(), and close() methods so that the
 * corresponding events are passed to the outer window.
 *
 * MdiSubWindow can serve as a wrapper for another widget. Use MdiSubWindow::setWidget(...) to set its internal widget.
 * In this case if close event needs to be processed override closeEvent(...) of the internal widget.
 *
 * \sa Folder, ApplicationWindow
 */
class MdiSubWindow: public MdiSubWindowParent_t
{
  Q_OBJECT

public:

  //! Constructor
  /**
   * @param parent :: parent window
   * @param label :: window label
   * @param name :: window name
   * @param f :: window flags
   * \sa setCaptionPolicy(), captionPolicy()
   */
  MdiSubWindow(QWidget *parent, const QString& label = QString(), const QString& name = QString(), Qt::WFlags f = 0);

  //! Possible window captions.
  enum CaptionPolicy{
    Name = 0, //!< caption determined by the window name
    Label = 1, //!< caption detemined by the window label
    Both = 2 //!< caption = "name - label"
  };
  enum Status{Hidden = -1, Normal = 0, Minimized = 1, Maximized = 2};

  /// Get the pointer to ApplicationWindow
  ApplicationWindow *applicationWindow() {return d_app;}

public slots:

  //! Return the window label
  QString windowLabel(){return QString(d_label);}
  //! Set the window label
  void setWindowLabel(const QString& s) { d_label = s; updateCaption();}

  //! Return the window name
  QString name(){return objectName();}
  //! Set the window name
  void setName(const QString& s){setObjectName(s); updateCaption();}

  //! Return the caption policy
  CaptionPolicy captionPolicy(){return d_caption_policy;}
  //! Set the caption policy
  void setCaptionPolicy(CaptionPolicy policy) { d_caption_policy = policy; updateCaption(); }

  //! Return the creation date
  QString birthDate(){return d_birthdate;}
  //! Set the creation date
  void setBirthDate(const QString& s){d_birthdate = s;}

  //! Return the window status as a string
  QString aspect();
  //! Return the window status flag (hidden, normal, minimized or maximized)
    Status status(){return d_status;}
  //! Set the window status flag (hidden, normal, minimized or maximized)
  void setStatus(Status s);

  // TODO:
  //! Not implemented yet
  virtual void restore(const QStringList& ){}

  virtual void exportPDF(const QString&){}

  virtual QString saveToString(const QString &, bool = false){return QString();}

  // TODO: make this return something useful
  //! Size of the widget as a string
  virtual QString sizeToString();

  //!Notifies that a window was hidden by a direct user action
  virtual void setHidden();

  //event handlers
  //! Close event handler
  /**
   * Ask the user "delete, hide, or cancel?" if the
   * "ask on close" flag is set.
   */
  void closeEvent( QCloseEvent *);
  void resizeEvent( QResizeEvent* );

  //! Toggle the "ask on close" flag
  void confirmClose(bool ask);

  //! Filters other object's events (customizes title bar's context menu)
  bool eventFilter(QObject *object, QEvent *e);

  FloatingWindow* getFloatingWindow() const;
  QMdiSubWindow* getDockedWindow() const;
  QWidget* getWrapperWindow() const;

  void setNormal();
  void setMinimized();
  void setMaximized();

  //! Returns the size the window had before a change state event to minimized.
  QSize minRestoreSize(){return d_min_restore_size;}

  //! Static function used as a workaround for ASCII files having end line char != '\n'.
  /*
   * It counts the number of valid rows to be imported and the number of first lines to be ignored.
   * It creates a temporary file with '\n' terminated lines which can be correctly read by QTextStream
   * and returnes a path to this file.
   */
  static QString parseAsciiFile(const QString& fname, const QString &commentString, int endLine,
                                int ignoreFirstLines, int maxRows, int& rows);

  void setconfirmcloseFlag(bool closeflag){d_confirm_close=closeflag;}

  //! Notifies the main application that the window has been modified
  void notifyChanges(){emit modifiedWindow(this);}
  virtual void print(){}

  bool close();
  void hide();
  void show();
  void resize(int w, int h);
  void resize(const QSize& size);
  QSize sizeHint() const;

  /// Focus on the window
  void setFocus();

  void move(int x, int y);
  void move(const QPoint& pos);
  /// Resize the window to it's default size
  void resizeToDefault();

public: //non-slot methods
  /**@name Floating/Docking */
  ///@{
  /// If docked, undock the window out of the MDI area
  void undock();
  /// Query if the window is floating
  bool isFloating() const;
  /// If floating, dock the window inside the MDI area of the application
  void dock();
  /// Query if the window is docked
  bool isDocked() const;
  /// Detach this window from any parent window
  void detach();
  ///@}

signals:
  //! Emitted when the window was closed
  void closedWindow(MdiSubWindow *);
  //! Emitted when the window was hidden
  void hiddenWindow(MdiSubWindow *);
  void modifiedWindow(MdiSubWindow *);
  void resizedWindow(MdiSubWindow *);
  //! Emitted when the window status changed
  void statusChanged(MdiSubWindow *);
  //! Show the context menu
  void showContextMenu();
  //! Emitted when window caption changed
  void captionChanged(QString, QString);
  //! Emitted when the window wants to dock to the MDI area
  void dockToMDIArea(MdiSubWindow *);
  //! Emitted when the window wants to undock
  void undockFromMDIArea(MdiSubWindow *);
  /// Emited to detach this window from any parent - docked or floating
  void detachFromParent(MdiSubWindow *);

  void dragMousePress(QPoint);
  void dragMouseRelease(QPoint);
  void dragMouseMove(QPoint);
  
protected:
  //! Catches status changes
  virtual void changeEvent(QEvent *event);

private:
  //! Used to parse ASCII files with carriage return ('\r') endline.
  static QString parseMacAsciiFile(const QString& fname, const QString &commentString,
                           int ignoreFirstLines, int maxRows, int& rows);
  //! Set caption according to current CaptionPolicy, name and label
  void updateCaption();
  /// Store the pointer to the ApplicationWindow
  ApplicationWindow *d_app;
  //! The window label
  /**
   * \sa setWindowLabel(), windowLabel(), setCaptionPolicy()
   */
  QString d_label;
  //! The window status
  Status d_status;
  //! The caption policy
  /**
   * \sa setCaptionPolicy(), captionPolicy()
   */
  CaptionPolicy d_caption_policy;
  //! Toggle on/off: Ask the user "delete, hide, or cancel?" on a close event
  bool d_confirm_close;
  //! The creation date
  QString d_birthdate;
  //! Stores the size the window had before a change state event to minimized.
  QSize d_min_restore_size;

  friend class FloatingWindow;
};

typedef QList<MdiSubWindow*> MDIWindowList;

#endif
