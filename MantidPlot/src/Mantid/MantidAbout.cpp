// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAbout.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "globals.h"

/**
 * Constructor
 * @param parent :: The parent widget
 */

MantidAbout::MantidAbout(QWidget *parent)
    : MantidQt::API::MantidDialog(parent) {
  m_uiForm.setupUi(this);

  QLabel *releasedate = m_uiForm.release_datevalue;
  QString releaseDate(Mantid::Kernel::MantidVersion::releaseDate());
  releasedate->setText(releaseDate);

  QString version(Mantid::Kernel::MantidVersion::version());
  QLabel *releaseversion = m_uiForm.release_versionvalue;
  releaseversion->setText(version);

  QString release(Mantid::Kernel::MantidVersion::revision());
  release.insert(0, "<p>");
  release.append(" (<a href=\"http://github.com/mantidproject/mantid/commit/");
  release.append(Mantid::Kernel::MantidVersion::revisionFull());
  release.append("\">on github</a>)</p>");
  QLabel *releaselabel = m_uiForm.revision_value;
  releaselabel->setText(release);
  connect(releaselabel, SIGNAL(linkActivated(const QString &)), this,
          SLOT(openExternalLink(const QString &)));

  QLabel *builtusing_labelvalue = m_uiForm.builtusing_labelvalue;
  QString builtusing = "QtiPlot " + QString::number(maj_version) + "." +
                       QString::number(min_version) + "." +
                       QString::number(patch_version) + extra_version + "  ";
  builtusing += "Released: " + QString(release_date) + "<br>";
  builtusing += QString(copyright_string);
  builtusing_labelvalue->setText(builtusing);

  QString mantidurl = "<p><a href = "
                      "http://www.mantidproject.org/Main_Page>http://"
                      "www.mantidproject.org</a></p>";
  QLabel *url = m_uiForm.mantidurl;
  url->setText(mantidurl);
  connect(url, SIGNAL(linkActivated(const QString &)), this,
          SLOT(openExternalLink(const QString &)));

  QString mantidDOI = QString::fromStdString(
      "<p><a href = " + Mantid::Kernel::MantidVersion::doi() + ">" +
      Mantid::Kernel::MantidVersion::doi() + "</a></p>");
  m_uiForm.mantiddoi->setText(mantidDOI);
  connect(m_uiForm.mantiddoi, SIGNAL(linkActivated(const QString &)), this,
          SLOT(openExternalLink(const QString &)));

  QString mantidCitation = QString::fromStdString(
      "<p><a href = " + Mantid::Kernel::MantidVersion::paperCitation() + ">" +
      Mantid::Kernel::MantidVersion::paperCitation() + "</a></p>");
  m_uiForm.mantidcitation->setText(mantidCitation);
  connect(m_uiForm.mantidcitation, SIGNAL(linkActivated(const QString &)), this,
          SLOT(openExternalLink(const QString &)));
}

void MantidAbout::openExternalLink(const QString &link) {
  MantidQt::API::MantidDesktopServices::openUrl(link);
}