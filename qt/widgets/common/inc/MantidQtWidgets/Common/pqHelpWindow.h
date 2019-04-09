/*=========================================================================

   Program: ParaView
   Module:    pqHelpWindow.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef __pqHelpWindow_h
#define __pqHelpWindow_h

#include "DllOption.h"
#include <QMainWindow>

class QHelpEngine;
class QToolButton;
class QUrl;
#if defined(USE_QTWEBKIT)
class QWebView;
using QWebEngineView = QWebView;
#else
#include <QWebEnginePage>
class QWebEngineView;

/// Mimic the WebKit class to emit linkClicked signal from the page
class DelegatingWebPage : public QWebEnginePage {
  Q_OBJECT

public:
  DelegatingWebPage(QObject *parent = nullptr) : QWebEnginePage(parent) {}

  bool acceptNavigationRequest(const QUrl &url,
                               QWebEnginePage::NavigationType type,
                               bool) override {
    if (type == QWebEnginePage::NavigationTypeLinkClicked) {
      emit linkClicked(url);
      if (url.scheme().startsWith("http") || url.toString().endsWith(".png")) {
        // We don't want to open web links or image hex within the help window
        return false;
      }
    }
    return true;
  }

signals:
  void linkClicked(const QUrl &);
};

#endif

/// pqHelpWindow provides a assistant-like window  for showing help provided by
/// a QHelpEngine.
class EXPORT_OPT_MANTIDQT_COMMON pqHelpWindow : public QMainWindow {
  Q_OBJECT
  using Superclass = QMainWindow;

public:
  pqHelpWindow(QHelpEngine *engine, QWidget *parent = nullptr,
               Qt::WindowFlags flags = nullptr);

public slots:
  /// Requests showing of a particular page. The url must begin with "qthelp:"
  /// scheme when referring to pages from the help files.
  virtual void showPage(const QString &url, bool linkClicked = false);
  virtual void showPage(const QUrl &url, bool linkClicked = false);
  /// Show a page linked to by another page in the help window
  virtual void showLinkedPage(const QUrl &url);

  /// Tries to locate a file name index.html in the given namespace and then
  /// shows that page.
  virtual void showHomePage(const QString &namespace_name);
  virtual void showHomePage();

  /// Prints the current open page
  virtual void printPage();

signals:
  /// fired to relay warning messages from the help system.
  void helpWarnings(const QString & /*_t1*/);

protected slots:
  void search();
  void linkHovered(const QString &link, const QString &title = "",
                   const QString &textContent = "");
  void updateNavButtons();

protected:
  QHelpEngine *m_helpEngine;
  QWebEngineView *m_browser;
  QToolButton *m_forward;
  QToolButton *m_backward;

private:
  Q_DISABLE_COPY(pqHelpWindow)
  void errorMissingPage(const QUrl &url);

  class pqNetworkAccessManager;
  friend class pqNetworkAccessManager;
};

#endif
