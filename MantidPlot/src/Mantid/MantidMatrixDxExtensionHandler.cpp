#include "MantidMatrixDxExtensionHandler.h"
#include "Preferences.h"
#include <QHeaderView>
#include <QTableView>

MantidMatrixDxExtensionHandler::MantidMatrixDxExtensionHandler()
    : m_type(MantidMatrixModel::Type::DX) {}

MantidMatrixDxExtensionHandler::~MantidMatrixDxExtensionHandler() {}

/**
 * Sets the number format for DX values
 * @param extension: a MantidMatrix extension
 * @param format: the format
 * @param precision: the precision
 */
void MantidMatrixDxExtensionHandler::setNumberFormat(
    MantidMatrixTabExtension &extension, const QChar &format, int precision) {
  if (extension.type == m_type) {
    extension.model->setFormat(format, precision);
  } else {
    m_successor->setNumberFormat(extension, format, precision);
  }
}

/**
 * Record the number format
 * @param extension: the extension to analyse
 * @param format: the format
 * @param precision: the precision
 */
void MantidMatrixDxExtensionHandler::recordFormat(
    MantidMatrixTabExtension &extension, const QChar &format, int precision) {
  if (extension.type == m_type) {
    MantidPreferences::MantidMatrixNumberFormatDx(format);
    MantidPreferences::MantidMatrixNumberPrecisionDx(precision);
  } else {
    m_successor->setNumberFormat(extension, format, precision);
  }
}

/**
 * Get the format
 * @param extension: the extension
 * @returns a format character
 */
QChar MantidMatrixDxExtensionHandler::getFormat(
    MantidMatrixTabExtension &extension) {
  if (extension.type == m_type) {
    return MantidPreferences::MantidMatrixNumberFormatDx();
  } else {
    return m_successor->getFormat(extension);
  }
}

/**
 * Get the precision
 * @param extension: the extension
 * @returns the precision
 */
int MantidMatrixDxExtensionHandler::getPrecision(
    MantidMatrixTabExtension &extension) {
  if (extension.type == m_type) {
    return MantidPreferences::MantidMatrixNumberPrecisionDx();
  } else {
    return m_successor->getPrecision(extension);
  }
}

/**
 * Set the column width
 * @param extension: the extension
 * @param width: the width
 * @param numberOfColumns: the number of columns
 */
void MantidMatrixDxExtensionHandler::setColumnWidth(
    MantidMatrixTabExtension &extension, int width, int numberOfColumns) {
  if (extension.type == m_type) {
    auto &table_view = extension.tableView;
    table_view->horizontalHeader()->setDefaultSectionSize(width);
    for (int i = 0; i < numberOfColumns; i++) {
      table_view->setColumnWidth(i, width);
    }
  } else {
    m_successor->setColumnWidth(extension, width, numberOfColumns);
  }
}

/**
 * Get the column width
 * @param extension: the extension
 * @returns the column width
 */
int MantidMatrixDxExtensionHandler::getColumnWidth(
    MantidMatrixTabExtension &extension) {
  if (extension.type == m_type) {
    auto &table_view = extension.tableView;
    return table_view->columnWidth(0);
  } else {
    return m_successor->getColumnWidth(extension);
  }
}

/**
 * Get the table view
 * @param extension: the extension
 * @returns a pointer to the table view
 */
QTableView *MantidMatrixDxExtensionHandler::getTableView(
    MantidMatrixTabExtension &extension) {
  if (extension.type == m_type) {
    return extension.tableView.get();
  } else {
    return m_successor->getTableView(extension);
  }
}

/**
 * Set the column width preference
 * @param extension: the extension
 * @param width: the width
 */
void MantidMatrixDxExtensionHandler::setColumnWidthPreference(
    MantidMatrixTabExtension &extension, int width) {
  if (extension.type == m_type) {
    MantidPreferences::MantidMatrixColumnWidthDx(width);
  } else {
    m_successor->setColumnWidthPreference(extension, width);
  }
}

/**
 * Get the column width preference
 */
int MantidMatrixDxExtensionHandler::getColumnWidthPreference(
    MantidMatrixTabExtension &extension) {
  if (extension.type == m_type) {
    return MantidPreferences::MantidMatrixColumnWidthDx();
  } else {
    return m_successor->getColumnWidthPreference(extension);
  }
}
