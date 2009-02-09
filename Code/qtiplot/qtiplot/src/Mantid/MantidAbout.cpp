#include "MantidAbout.h"
#include "MantidPlotReleaseDate.h"
//#include "../globals.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

const int maj_version = 0;
const int min_version = 9;
const int patch_version = 5;
extern const char * extra_version;
extern const char * copyright_string;
extern const char * release_date;

MantidAbout::MantidAbout(QWidget *parent):QDialog(parent)
{
    
    setAttribute(Qt::WA_DeleteOnClose);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *closeButton = new QPushButton("OK");
    connect(closeButton,SIGNAL(clicked()),this,SLOT(close()));
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();

    QLabel *mantidLogoLabel = new QLabel();
    QPixmap *mantidLogoPixmap = new QPixmap(":/Mantid Logo.png");
    mantidLogoLabel->setPixmap(*mantidLogoPixmap);
    QHBoxLayout *mantidLayout = new QHBoxLayout;
    mantidLayout->addStretch();
    mantidLayout->addWidget(mantidLogoLabel);
    mantidLayout->addStretch();

    QString str = "<h2> MantidPlot</h2> <h3> release date: " + QString(MANTIDPLOT_RELEASE_DATE) + QString("</h3>");
    str += "Built using";
    str += "<h3>QtiPlot " + QString::number(maj_version) + "." +
		QString::number(min_version) + "." + QString::number(patch_version) + extra_version + "  ";
    str += "Released: " + QString(release_date) + "<br>";
    str += QString(copyright_string).replace("\n", "<br>") + "</h3>";

    str += "<h3>Mantid</h3><p><a href = http://www.mantidproject.org/Main_Page>http://www.mantidproject.org</a></p>";
    QLabel *mantidPlotLabel = new QLabel(str);
    mantidPlotLabel->setOpenExternalLinks(true);

    QLabel *isisLogoLabel = new QLabel;
    QLabel *tessellaLogoLabel = new QLabel;
    QPixmap *isisLogoPixmap = new QPixmap(":/ISIS Logo.gif");
    QPixmap *tessellaLogoPixmap = new QPixmap(":/Tessella_logo_intranet.gif");
    isisLogoLabel->setPixmap(*isisLogoPixmap);
    tessellaLogoLabel->setPixmap(*tessellaLogoPixmap);
    QHBoxLayout *logosLayout = new QHBoxLayout;
    logosLayout->addWidget(isisLogoLabel);
    logosLayout->addWidget(tessellaLogoLabel);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(mantidLayout);
    layout->addWidget(mantidPlotLabel);
    layout->addLayout(logosLayout);
    layout->addLayout(buttonLayout);
	setLayout(layout);
	setWindowTitle("Mantid - About");
	setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));

}

MantidAbout::~MantidAbout()
{
}

