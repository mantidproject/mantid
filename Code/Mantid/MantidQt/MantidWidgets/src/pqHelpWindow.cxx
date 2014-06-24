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

#include <QByteArray>
#include <QByteArray>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHelpContentItem>
#include <QHelpContentModel>
#include <QHelpContentWidget>
#include <QHelpEngine>
#include <QHelpIndexWidget>
#include <QHelpSearchEngine>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#include <QPointer>
#include <QtDebug>
#include <QStringList>
#include <QTextBrowser>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QLabel>
#include <iostream> // REMOVE

# include <QNetworkReply>
# include <QNetworkAccessManager>
# include <QNetworkProxy>
# include <QWebPage>
# include <QWebView>

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
    std::cout << "scheme = " << url.scheme().toStdString() << " url = " << url.toString().toStdString() << std::endl;
    if(url.scheme() == "qthelp")
      return QVariant(m_helpEngine->fileData(url));
    return QTextBrowser::loadResource(type, url);
  }

private:
  QHelpEngine* m_helpEngine;
};

/*
class SearchResultWidget : public QTreeWidget
{
    Q_OBJECT

public:
    SearchResultWidget(QWidget *parent = 0)
        : QTreeWidget(parent)
    {
        header()->hide();
        connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(itemActivated(QTreeWidgetItem*,int)));
    }

    void showResultPage(const QList<QHelpSearchEngine::SearchHit> hits)
    {
        foreach (const QHelpSearchEngine::SearchHit &hit, hits)
            new QTreeWidgetItem(this, QStringList(hit.first) << hit.second);
    }

signals:
    void requestShowLink(const QUrl &url);

private slots:
    void itemActivated(QTreeWidgetItem *item, int column)
    {
        if (item) {
            QString data = item->data(1, Qt::DisplayRole).toString();
            emit requestShowLink(data);
        }
    }
};
*/

class SearchResultWidget : public QTextBrowser
{
public:
  SearchResultWidget(QWidget *parent = 0)
    : QTextBrowser(parent)
  {
    connect(this, SIGNAL(anchorClicked(QUrl)),
            this, SIGNAL(requestShowLink(QUrl)));
    setContextMenuPolicy(Qt::NoContextMenu);
  }

  void showResultPage(const QList<QHelpSearchEngine::SearchHit> hits, bool isIndexing)
  {
    QString htmlFile = QString(QLatin1String("<html><head><title>%1</title></head><body>"))
        .arg(tr("Search Results"));

    int count = hits.count();
    if (count != 0) {
      if (isIndexing)
        htmlFile += QString(QLatin1String("<div style=\"text-align:left; font-weight:bold; color:red\">"
                                          "%1&nbsp;<span style=\"font-weight:normal; color:black\">"
                                          "%2</span></div></div><br>")).arg(tr("Note:"))
            .arg(tr("The search results may not be complete since the "
                    "documentation is still being indexed!"));

      foreach (const QHelpSearchEngine::SearchHit &hit, hits) {
        htmlFile += QString(QLatin1String("<div style=\"text-align:left; font-weight:bold\""
                                          "><a href=\"%1\">%2</a><div style=\"color:green; font-weight:normal;"
                                          " margin:5px\">%1</div></div><p></p>"))
            .arg(hit.first).arg(hit.second);
      }
    } else {
      htmlFile += QLatin1String("<div align=\"center\"><br><br><h2>")
          + tr("Your search did not match any documents.")
          + QLatin1String("</h2><div>");
      if (isIndexing)
        htmlFile += QLatin1String("<div align=\"center\"><h3>")
            + tr("(The reason for this might be that the documentation "
                 "is still being indexed.)")
            + QLatin1String("</h3><div>");
    }

    htmlFile += QLatin1String("</body></html>");

    setHtml(htmlFile);
  }

signals:
  void requestShowLink(const QUrl &url);

private slots:
  void setSource(const QUrl & /* name */) {}
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

  QObject::connect(this->m_helpEngine, SIGNAL(warning(const QString&)),
    this, SIGNAL(helpWarnings(const QString&)));

  this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  this->tabifyDockWidget(ui.contentsDock, ui.indexDock);
  this->tabifyDockWidget(ui.indexDock, ui.searchDock);
  ui.contentsDock->setWidget(this->m_helpEngine->contentWidget());
  ui.indexDock->setWidget(this->m_helpEngine->indexWidget());
  ui.contentsDock->raise();

  QWidget* searchPane = new QWidget(this);
  QVBoxLayout* vbox = new QVBoxLayout();
  searchPane->setLayout(vbox);
  vbox->addWidget(this->m_helpEngine->searchEngine()->queryWidget());
//  vbox->addWidget(this->m_helpEngine->searchEngine()->resultWidget());

  m_resultsWidget = new SearchResultWidget(this);
  vbox->addWidget(m_resultsWidget);
  connect(m_resultsWidget, SIGNAL(requestShowLink(QUrl)), this,
      SIGNAL(requestShowLink(QUrl)));
//  std::cout << "resultWidget:" << this->m_helpEngine->searchEngine()->resultWidget() << std::endl;

  ui.searchDock->setWidget(searchPane);
  connect(this->m_helpEngine->searchEngine()->queryWidget(), SIGNAL(search()),
          this, SLOT(search()));
//  QObject::connect(this->m_helpEngine->searchEngine(), SIGNAL(searchingStarted()),
//                   this, SLOT(searchStarted()));
  QObject::connect(this->m_helpEngine->searchEngine(), SIGNAL(searchingFinished(int)),
                   this, SLOT(searchFinished(int)));
//  connect(this->m_helpEngine->searchEngine()->resultWidget(),
//          SIGNAL(requestShowLink(const QUrl&)),
//          this, SLOT(showPage(const QUrl&)));

  connect(this->m_helpEngine->indexWidget(), SIGNAL(linkActivated(QUrl,QString)),
          this, SLOT(showPage(QUrl)));

  this->m_browser = new HelpBrowser(m_helpEngine, this);
  connect(this->m_browser, SIGNAL(anchorClicked(QUrl)), this, SLOT(showPage(QUrl)));
  this->setCentralWidget(this->m_browser);

    
  QObject::connect(
    this->m_helpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl&)),
    this, SLOT(showPage(const QUrl&)));
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
  std::cout << "showPage(QUrl=" << url.toString().toStdString() << ")" << std::endl;
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
  std::cout << "search()" << std::endl;
//  m_helpEngine->searchEngine()->search(m_helpEngine->searchEngine()->queryWidget()->query());

  QList<QHelpSearchQuery> query =
    this->m_helpEngine->searchEngine()->queryWidget()->query();
  foreach (auto term, query)
  {
    std::cout << "( ";
    QStringList items = term.wordList;
    foreach (auto item, items)
    {
      std::cout << item.toStdString() << " ";
    }
    std::cout << ")";
  }
  std::cout << std::endl;
  this->m_helpEngine->searchEngine()->search(query);
}

//-----------------------------------------------------------------------------
void pqHelpWindow::searchStarted()
{
  std::cout << "searchStarted()" << std::endl;
}

//-----------------------------------------------------------------------------
void pqHelpWindow::searchFinished(int hits)
{
  std::cout << "searchFinished(" << hits << ")" << std::endl;
  m_resultsWidget->showResultPage(m_helpEngine->searchEngine()->hits(0, hits), false);

//  QList<QPair<QString, QString> > hitList = m_helpEngine->searchEngine()->hits(0, hits - 1);
//  QString html = "<html><body>";
//  for(int i = 0; i < hitList.size(); i++)
//  {
//    QString url = hitList[i].first;
//    QString title = hitList[i].second;
//    html += "<a href=\"" + url + "\">" + title + "</a><br>";
//  }
//  html += "</body></html>";
//  m_helpEngine->searchEngine()->resultWidget()->setHtml(html);
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
  qWarning() << "Could not locate index.html";
}
