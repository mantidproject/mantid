// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TEXTFILEIO_H_
#define TEXTFILEIO_H_

#include <QString>
#include <QStringList>

/**
 * Defines a static class for simple text file
 * I/O.
 */
class TextFileIO {
public:
  /// Construct the object with a list of file filters
  explicit TextFileIO(QStringList fileFilters = QStringList());

  /// Save to a file
  bool save(const QString &txt, const QString &filename) const;

private:
  /// Open a save dialog
  QString askWhereToSave() const;

  const QStringList m_filters;
};

#endif /* TEXTFILEIO_H_ */
