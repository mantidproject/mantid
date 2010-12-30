#include "MantidAbout.h"
#include "MantidPlotReleaseDate.h"
//#include "../globals.h"

extern const int maj_version ;
extern const int min_version ;
extern const int patch_version ;
const int maj_version = 0;
const int min_version = 9;
const int patch_version = 5;
extern const char * extra_version;
extern const char * copyright_string;
extern const char * release_date;

/**
 * Constructor
 * @param parent The parent widget
 */

MantidAbout::MantidAbout(QWidget *parent) : MantidQt::API::MantidDialog(parent)
{
	m_uiForm.setupUi(this);

	QLabel* releasedate=m_uiForm.release_datevalue;
	QString temp (MANTIDPLOT_RELEASE_DATE);
	QStringList releaseDate=temp.split("(");
	releasedate->setText(releaseDate[0]);
	
	QString version(MANTIDPLOT_RELEASE_VERSION);
	QLabel* releaseversion=m_uiForm.release_versionvalue;
	releaseversion->setText(version);
	
	QLabel* builtusing_labelvalue=m_uiForm.builtusing_labelvalue;
	QString builtusing="QtiPlot "+QString::number(maj_version) + "." +
		QString::number(min_version) + "." + QString::number(patch_version) + extra_version + "  ";
    builtusing += "Released: " + QString(release_date) + "<br>";
    builtusing += QString(copyright_string);
	builtusing_labelvalue->setText(builtusing);

	QString mantidurl="<p><a href = http://www.mantidproject.org/Main_Page>http://www.mantidproject.org</a></p>";
	QLabel* url=m_uiForm.mantidurl;
	url->setText(mantidurl);
	url->setOpenExternalLinks(true);


	
}

