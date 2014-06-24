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
#include "MantidQtMantidWidgets/pqHelpWindow.h"
#include "ui_pqHelpWindow.h"

#include <QDesktopServices>
#include <QHelpContentWidget>
#include <QHelpEngine>
#include <QHelpIndexWidget>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#include <QTextBrowser>
#include <QTextStream>
#include <QUrl>

class HelpBrowser : public QTextBrowser
{
public:
  HelpBrowser(QHelpEngine* helpEngine, QWidget* parent) :
    QTextBrowser(parent),
    m_helpEngine(helpEngine)
  {
    // use the signals to navigate so we can interfere
    setOpenLinks(false);

    // add css files
    addCss("_static/basic.css");
    addCss("_static/pygments.css");
    addCss("_static/bootstrap-3.1.0/css/bootstrap.min.css");
    addCss("_static/bootstrap-3.1.0/css/bootstrap-theme.min.css");
    addCss("_static/bootstrap-sphinx.css");
    addCss("_static/custom.css");
  }

  /**
   * Load an individual css file to the document.
   * @param filename the css file to load.
   */
  void addCss(const std::string &filename)
  {
    QString name(":");
    name += filename.c_str();
    QFile file(name);
    if (file.open(QIODevice::ReadOnly))
    {
      QTextStream stream(&file);
      QString css;
      while(!stream.atEnd())
        css += stream.readLine();

      file.close();

      document()->addResource(QTextDocument::StyleSheetResource, QUrl(filename.c_str()), css);
    }
  }

  virtual QVariant loadResource(int type, const QUrl &url)
  {
    if(url.scheme() == "qthelp")
      return QVariant(m_helpEngine->fileData(url));
    return QTextBrowser::loadResource(type, url);
  }

private:
  QHelpEngine* m_helpEngine;
};

// ****************************************************************************
//            CLASS pqHelpWindow
// ****************************************************************************

//-----------------------------------------------------------------------------
pqHelpWindow::pqHelpWindow(
  QHelpEngine* engine, QWidget* parentObject, Qt::WindowFlags parentFlags)
  : Superclass(parentObject, parentFlags), m_helpEngine(engine)
{
  Q_ASSERT(engine != NULL);

  Ui::pqHelpWindow ui;
  ui.setupUi(this);

  // all warnings from the help engine get logged
  QObject::connect(this->m_helpEngine, SIGNAL(warning(const QString&)),
                   this, SIGNAL(helpWarnings(const QString&)));

  this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  // get contents and index added
  this->tabifyDockWidget(ui.contentsDock, ui.indexDock);
  this->tabifyDockWidget(ui.indexDock, ui.searchDock);
  ui.contentsDock->setWidget(this->m_helpEngine->contentWidget());
  ui.indexDock->setWidget(this->m_helpEngine->indexWidget());
  ui.contentsDock->raise();

  // setup the search tab
  QWidget* searchPane = new QWidget(this);
  QVBoxLayout* vbox = new QVBoxLayout();
  searchPane->setLayout(vbox);
  vbox->addWidget(this->m_helpEngine->searchEngine()->queryWidget());
  vbox->addWidget(this->m_helpEngine->searchEngine()->resultWidget());
  connect(this->m_helpEngine->searchEngine()->resultWidget(), SIGNAL(requestShowLink(QUrl)),
          this, SLOT(showPage(QUrl)));

  // set the
  ui.searchDock->setWidget(searchPane);
  connect(this->m_helpEngine->searchEngine()->queryWidget(), SIGNAL(search()),
          this, SLOT(search()));

  // connect the index page to the content pane
  connect(this->m_helpEngine->indexWidget(), SIGNAL(linkActivated(QUrl,QString)),
          this, SLOT(showPage(QUrl)));

  // custom browser for rendering help pages
  this->m_browser = new HelpBrowser(m_helpEngine, this);
  connect(this->m_browser, SIGNAL(anchorClicked(QUrl)), this, SLOT(showPage(QUrl)));
  this->setCentralWidget(this->m_browser);
  QObject::connect(
    this->m_helpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl&)),
    this, SLOT(showPage(const QUrl&)));

  // setup the search engine to do its job
  m_helpEngine->searchEngine()->reindexDocumentation();
}

//-----------------------------------------------------------------------------
pqHelpWindow::~pqHelpWindow()
{
}

/**
 * Set the contents of the browser to show an error message.
 * @param url The url that could not be found.
 */
void pqHelpWindow::errorMissingPage(const QUrl& url)
{
  QString htmlDoc = QString(QLatin1String("<html><head><title>Invalid Url - %1</title></head><body>"))
      .arg(url);

  htmlDoc += QString(QLatin1String("<center><h1>Missing page - %1</h1></center>"))
      .arg(url);

  htmlDoc += QLatin1String("</body></html>");

  m_browser->setHtml(htmlDoc);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showPage(const QString& url)
{
  this->showPage(QUrl(url));
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showPage(const QUrl& url)
{
  if(url.scheme() == "qthelp")
  {
    if (this->m_helpEngine->findFile(url).isValid())
      this->m_browser->setSource(url);
    else
      errorMissingPage(url);
  }
  else
  {
    QDesktopServices::openUrl(url);
  }
}

//-----------------------------------------------------------------------------
void pqHelpWindow::search()
{
  QList<QHelpSearchQuery> query =
      this->m_helpEngine->searchEngine()->queryWidget()->query();
  this->m_helpEngine->searchEngine()->search(query);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::showHomePage(const QString& namespace_name)
{
  QList<QUrl> html_pages = this->m_helpEngine->files(namespace_name,
                                                     QStringList(), "html");
  // now try to locate a file named index.html in this collection.
  foreach (QUrl url, html_pages)
  {
    if (url.path().endsWith("index.html"))
    {
      this->showPage(url.toString());
      return;
    }
  }
  errorMissingPage(QUrl("Could not locate index.html"));
}
