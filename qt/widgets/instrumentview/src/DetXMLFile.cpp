#include "MantidQtWidgets/InstrumentView/DetXMLFile.h"

#include <QTemporaryFile>
#include <QDir>

#include <fstream>

namespace MantidQt {
namespace MantidWidgets {
/**
* Create a grouping file to extract all detectors in detector_list excluding
* those in exclude.
* @param detector_list :: List of detector ids to include in the grouping file.
* @param exclude :: List of detector ids which if founfd in detector_list to be
* excluded from grouping.
* @param fname :: Name of the file to save the grouping to.
*/
DetXMLFile::DetXMLFile(const std::vector<int> &detector_list,
                       const QList<int> &exclude, const QString &fname) {
  m_fileName = fname;
  m_delete = false;
  std::ofstream out(m_fileName.toStdString().c_str());
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
  out << R"(<group name="sum"> <detids val=")";
  std::vector<int>::const_iterator idet = detector_list.begin();
  for (; idet != detector_list.end(); ++idet) {
    if (!exclude.contains(*idet)) {
      out << *idet << ',';
    }
  }
  out << "\"/> </group> \n</detector-grouping>\n";
}

/**
* Create a grouping file to extract detectors in dets. Option List - one group -
* one detector,
* Option Sum - one group which is a sum of the detectors
* If fname is empty create a temporary file
*/
DetXMLFile::DetXMLFile(const QList<int> &dets, Option opt,
                       const QString &fname) {
  if (dets.empty()) {
    m_fileName = "";
    m_delete = false;
    return;
  }

  if (fname.isEmpty()) {
    QTemporaryFile mapFile;
    mapFile.open();
    m_fileName = mapFile.fileName() + ".xml";
    mapFile.close();
    m_delete = true;
  } else {
    m_fileName = fname;
    m_delete = false;
  }

  switch (opt) {
  case Sum:
    makeSumFile(dets);
    break;
  case List:
    makeListFile(dets);
    break;
  }
}

/// Make grouping file where each detector is put into its own group
void DetXMLFile::makeListFile(const QList<int> &dets) {
  std::ofstream out(m_fileName.toStdString().c_str());
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
  foreach (int det, dets) {
    out << "<group name=\"" << det << "\"> <detids val=\"" << det
        << "\"/> </group> \n";
  }
  out << "</detector-grouping>\n";
}

/// Make grouping file for putting the detectors into one group (summing the
/// detectors)
void DetXMLFile::makeSumFile(const QList<int> &dets) {
  std::ofstream out(m_fileName.toStdString().c_str());
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
  out << R"(<group name="sum"> <detids val=")";
  foreach (int det, dets) { out << det << ','; }
  out << "\"/> </group> \n</detector-grouping>\n";
}

/**
* Destructor. Removes the temporary file.
*/
DetXMLFile::~DetXMLFile() {
  if (m_delete) {
    QDir dir;
    dir.remove(m_fileName);
  }
}

} // MantidWidgets
} // MantidQt
