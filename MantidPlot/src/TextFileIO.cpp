// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "TextFileIO.h"
#include "MantidQtWidgets/Common/FileDialogHandler.h"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

/**
 * Construct an object with a list of file filters
 */
TextFileIO::TextFileIO(QStringList fileFilters) : m_filters(fileFilters) {}

/**
 * Save to a file
 * @param filename An optional filename
 * @return True if the save was successful, false otherwise
 */
bool TextFileIO::save(const QString &txt, const QString &filename) const {
  QString saved = filename;
  if (saved.isEmpty()) {
    saved = askWhereToSave();
  }
  if (saved.isEmpty())
    return true; // cancelled

  QFile file(saved);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(
        nullptr, "MantidPlot - File error",
        QString("Could not open file \"%1\" for writing.").arg(saved));
    return false;
  }

  QTextStream writer(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  writer << txt;
  QApplication::restoreOverrideCursor();
  file.close();
  return true;
}

/// Open a save dialog
QString TextFileIO::askWhereToSave() const {
  QString selectedFilter;
  QString filter = m_filters.join(";;");
  QString filename = QFileDialog::getSaveFileName(nullptr, "MantidPlot - Save",
                                                  "", filter, &selectedFilter);
  return MantidQt::API::FileDialogHandler::addExtension(filename,
                                                        selectedFilter);
}
