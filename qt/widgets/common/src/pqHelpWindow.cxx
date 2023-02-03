/*=========================================================================

   Program: ParaView
   Module:    pqHelpWindow.cxx

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
#include "MantidQtWidgets/Common/pqHelpWindow.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "ui_pqHelpWindow.h"

#include <QBuffer>
#include <QFileInfo>
#include <QHash>
#include <QHelpEngine>
#include <QHelpIndexWidget>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#include <QMimeDatabase>
#include <QMimeType>
#include <QNetworkProxy>
#include <QPointer>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

namespace {
/// Prefix for qthelp scheme
constexpr auto QTHELP_SCHEME = "qthelp";
} // namespace

#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>

/// Register on the scheme on library load as it must be done before
/// QApplication is created
struct QtHelpSchemeRegistration {
  QtHelpSchemeRegistration() {
    auto scheme = QWebEngineUrlScheme(QTHELP_SCHEME);
    scheme.setFlags(QWebEngineUrlScheme::LocalScheme | QWebEngineUrlScheme::LocalAccessAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
  }
};

const QtHelpSchemeRegistration QTHELP_REGISTRATION;

/// Adds support for qthelp scheme links that load content from them QHelpEngine
class QtHelpUrlHandler : public QWebEngineUrlSchemeHandler {
public:
  QtHelpUrlHandler(QHelpEngineCore *helpEngine, QObject *parent = nullptr)
      : QWebEngineUrlSchemeHandler(parent), m_helpEngine(helpEngine) {}

protected:
  void requestStarted(QWebEngineUrlRequestJob *request) override {
    const auto url = request->requestUrl();
    const auto resourceType = contentType(url);
    const auto array = m_helpEngine->fileData(url);
    QBuffer *buffer = new QBuffer;
    buffer->setData(array);
    buffer->open(QIODevice::ReadOnly);
    connect(buffer, &QIODevice::aboutToClose, buffer, &QObject::deleteLater);
    request->reply(resourceType.toLocal8Bit(), buffer);
  }

private:
  /**
   * Given a url return the content type of the resource based on the extension
   * @param url A url pointing to a resource
   */
  QString contentType(const QUrl &url) {
    QMimeDatabase mimeTypes;
    return mimeTypes.mimeTypeForFile(url.path(), QMimeDatabase::MatchExtension).name();
  }

private:
  QHelpEngineCore *m_helpEngine;
};

// ****************************************************************************
//            CLASS pqHelpWindow
// ****************************************************************************

//-----------------------------------------------------------------------------
pqHelpWindow::pqHelpWindow(QHelpEngine *engine, QWidget *parentObject, const Qt::WindowFlags &parentFlags)
    : Superclass(parentObject, parentFlags), m_helpEngine(engine) {
  Q_ASSERT(engine != nullptr);
  // Take ownership of the engine
  m_helpEngine->setParent(this);

  Ui::pqHelpWindow ui;
  ui.setupUi(this);

  // all warnings from the help engine get logged
  QObject::connect(this->m_helpEngine, SIGNAL(warning(const QString &)), this, SIGNAL(helpWarnings(const QString &)));

  // add a navigation toolbar
  auto *navigation = new QToolBar("Navigation");
  auto *home = new QPushButton("Home");
  auto *print = new QPushButton("Print...");
  print->setToolTip("Print the current page");

  m_forward = new QToolButton();
  m_forward->setArrowType(Qt::RightArrow);
  m_forward->setToolTip("next");
  m_forward->setEnabled(false);
  m_forward->setAutoRaise(true);

  m_backward = new QToolButton();
  m_backward->setArrowType(Qt::LeftArrow);
  m_backward->setToolTip("previous");
  m_backward->setEnabled(false);
  m_backward->setAutoRaise(true);

  navigation->addWidget(home);
  navigation->addWidget(print);
  navigation->addWidget(m_backward);
  navigation->addWidget(m_forward);
  navigation->setAllowedAreas(Qt::TopToolBarArea | Qt::RightToolBarArea);
  this->addToolBar(navigation);

  this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  // create index and search dock tabs
  this->tabifyDockWidget(ui.indexDock, ui.searchDock);
  ui.indexDock->setWidget(this->m_helpEngine->indexWidget());

  // setup the search tab
  auto *searchPane = new QWidget(this);
  auto *vbox = new QVBoxLayout();
  searchPane->setLayout(vbox);
  vbox->addWidget(this->m_helpEngine->searchEngine()->queryWidget());
  vbox->addWidget(this->m_helpEngine->searchEngine()->resultWidget());
  connect(this->m_helpEngine->searchEngine()->resultWidget(), SIGNAL(requestShowLink(QUrl)), this,
          SLOT(showPage(QUrl)));

  // set the search connection
  ui.searchDock->setWidget(searchPane);
  connect(this->m_helpEngine->searchEngine()->queryWidget(), SIGNAL(search()), this, SLOT(search()));

  // connect the index page to the content pane
  connect(this->m_helpEngine->indexWidget(), SIGNAL(linkActivated(QUrl, QString)), this, SLOT(showPage(QUrl)));

  // setup the content pane
  QWebEngineProfile::defaultProfile()->installUrlSchemeHandler(QTHELP_SCHEME, new QtHelpUrlHandler(engine, this));
  m_browser = new QWebEngineView(this);
  m_browser->setPage(new DelegatingWebPage(m_browser));
  connect(m_browser->page(), SIGNAL(linkClicked(QUrl)), this, SLOT(showLinkedPage(QUrl)));
  // set up the status bar
  connect(m_browser->page(), SIGNAL(linkHovered(QString)), this, SLOT(linkHovered(QString)));
  this->setCentralWidget(this->m_browser);

  // connect the navigation buttons
  connect(home, SIGNAL(clicked()), this, SLOT(showHomePage()));
  connect(print, SIGNAL(clicked()), this, SLOT(printPage()));
  connect(m_forward, SIGNAL(clicked()), m_browser, SLOT(forward()));
  connect(m_backward, SIGNAL(clicked()), m_browser, SLOT(back()));
  connect(m_forward, SIGNAL(clicked()), this, SLOT(updateNavButtons()));
  connect(m_backward, SIGNAL(clicked()), this, SLOT(updateNavButtons()));

  // setup the search engine to do its job
  m_helpEngine->searchEngine()->reindexDocumentation();
}

//-----------------------------------------------------------------------------

/**
 * Set the contents of the browser to show an error message.
 * @param url The url that could not be found.
 */
void pqHelpWindow::errorMissingPage(const QUrl &url) {
  QString htmlDoc =
      QString(QLatin1String("<html><head><title>Invalid Url - %1</title></head><body>")).arg(url.toString());

  htmlDoc += QString(QLatin1String("<center><h1>Missing page - %1</h1></center>")).arg(url.toString());

  htmlDoc += QLatin1String("</body></html>");

  m_browser->setHtml(htmlDoc);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showPage(const QString &url, bool linkClicked /* = false */) {
  this->showPage(QUrl::fromUserInput(url), linkClicked);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showPage(const QUrl &url, bool linkClicked /* = false */) {
  if (url.scheme() == QTHELP_SCHEME) {
    if (this->isExistingPage(url)) {
      if (!linkClicked)
        this->m_browser->setUrl(url);
    } else {
      errorMissingPage(url);
    }
    if (m_browser->history()->count() > 0)
      m_backward->setEnabled(true);
    m_forward->setEnabled(false);
  } else {
    using MantidQt::API::MantidDesktopServices;
    MantidDesktopServices::openUrl(url);
  }
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showLinkedPage(const QUrl &url) { this->showPage(url, true); }

//-----------------------------------------------------------------------------
void pqHelpWindow::printPage() {
  auto *printer = new QPrinter();
  QPrintDialog dialog(printer, this);
  dialog.setWindowTitle(tr("Print Document"));
  if (dialog.exec() != QDialog::Accepted)
    return;
  m_browser->page()->print(printer, [printer](bool) { printer->~QPrinter(); });
}

//-----------------------------------------------------------------------------
void pqHelpWindow::updateNavButtons() {
  m_forward->setEnabled(m_browser->history()->canGoForward());
  m_backward->setEnabled(m_browser->history()->canGoBack());
}

//-----------------------------------------------------------------------------
void pqHelpWindow::search() {
  auto search = this->m_helpEngine->searchEngine()->queryWidget()->searchInput();
  this->m_helpEngine->searchEngine()->search(search);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::linkHovered(const QString &link, const QString &title, const QString &textContent) {
  (void)title;
  (void)textContent;
  this->statusBar()->showMessage(link);
}

void pqHelpWindow::showHomePage() { showPage(QString("qthelp://org.mantidproject/doc/index.html")); }

//-----------------------------------------------------------------------------
void pqHelpWindow::showHomePage(const QString &namespace_name) {
  QList<QUrl> html_pages = this->m_helpEngine->files(namespace_name, QStringList(), "html");
  // now try to locate a file named index.html in this collection.
  foreach (QUrl url, html_pages) {
    if (url.path().endsWith("index.html")) {
      this->showPage(url.toString());
      return;
    }
  }
  errorMissingPage(QUrl("Could not locate index.html"));
}

//-----------------------------------------------------------------------------
bool pqHelpWindow::isExistingPage(const QUrl &url) {
  return (this->m_helpEngine->findFile(url).isValid() && (this->m_helpEngine->fileData(url).size() > 0));
}
