#ifndef MANTIDUI_H
#define MANTIDUI_H

#include "../ApplicationWindow.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/LoadRaw.h"

#include <QDockWidget>
#include <QTreeWidget>

class MantidUI;

class MantidDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    MantidDockWidget(QWidget*w):QDockWidget(w){}
    MantidDockWidget(MantidUI *mui, ApplicationWindow *w);
    void update();
protected:
    QTreeWidget *m_tree;
    friend class MantidUI;
private:
    QPushButton *m_loadButton;
    QPushButton *m_deleteButton;
    MantidUI *m_mantidUI;
};

class MantidUI:public QObject
{

    Q_OBJECT

public:
    MantidUI(ApplicationWindow *aw);
    void init();
    QStringList getWorkspaceNames();
    QStringList getAlgorithmNames();
    Mantid::API::Workspace_sptr LoadIsisRawFile(const QString& fileName, const QString& workspaceName);
    Mantid::API::IAlgorithm* CreateAlgorithm(const QString& algName);
    bool deleteWorkspace(const QString& workspaceName);
    ApplicationWindow *appWindow(){return m_appWindow;}
    QString getSelectedWorkspaceName();
    Mantid::API::Workspace_sptr getSelectedWorkspace();
    int getBinNumber(const QString& workspaceName);
    int getHistogramNumber(const QString& workspaceName);

    void update();

public slots:

    void tst();
    void loadWorkspace();
    void deleteWorkspace();

public:
    ApplicationWindow *m_appWindow;
    QMdiArea *d_workspace;// ApplicationWindow's private member
    MantidDockWidget *m_exploreMantid;
};

static const char * mantid_matrix_xpm[] = { 
"13 12 20 1",
" 	c None",
".	c #000821",
"+	c #000720",
"@	c #000927",
"#	c #000C39",
"$	c #000C37",
"%	c #000929",
"&	c #00041B",
"*	c #000519",
"=	c #597384",
"-	c #5A7387",
";	c #00061E",
">	c #4C7799",
",	c #517595",
"'	c #000212",
")	c #000209",
"!	c #000207",
"~	c #00030C",
"{	c #010101",
"]	c #8080FF",
".++@#$%++%#$&",
"*=-;>>;==;>,'",
")!))~~~!))~){",
"{]]{]]{]]{]]{",
"{]]{]]{]]{]]{",
"{{{{{{{{{{{{{",
"{]]{]]{]]{]]{",
"{]]{]]{]]{]]{",
"{{{{{{{{{{{{{",
"{]]{]]{]]{]]{",
"{]]{]]{]]{]]{",
"{{{{{{{{{{{{{"};

#endif
