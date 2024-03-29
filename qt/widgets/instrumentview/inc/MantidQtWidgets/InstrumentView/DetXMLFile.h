// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QList>
#include <QString>

#include <vector>

namespace MantidQt {
namespace MantidWidgets {
/**

A class for creating grouping xml files

*/
class DetXMLFile {
public:
  enum Option { List, Sum };
  /// Create a grouping file to extract all detectors in detector_list excluding
  /// those in exclude
  DetXMLFile(const std::vector<int> &detector_list, const std::vector<int> &exclude, const QString &fname);

  /// Create a grouping file to extract detectors in dets. Option List - one
  /// group - one detector,
  /// Option Sum - one group which is a sum of the detectors
  /// If fname is empty create a temporary file
  DetXMLFile(const std::vector<int> &dets, Option opt = List, const QString &fname = "");

  /// Destructor
  ~DetXMLFile();

  /// Make grouping file where each detector is put into its own group
  void makeListFile(const std::vector<int> &dets);

  /// Make grouping file for putting the detectors into one group (summing the
  /// detectors)
  void makeSumFile(const std::vector<int> &dets);

  /// Return the name of the created grouping file
  const std::string operator()() const { return m_fileName.toStdString(); }

private:
  QString m_fileName; ///< holds the grouping file name
  bool m_delete;      ///< if true delete the file on destruction
};

} // namespace MantidWidgets
} // namespace MantidQt
