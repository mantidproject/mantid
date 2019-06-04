// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMatrixExtensionRequest.h"
#include "MantidKernel/Logger.h"

#include "MantidMatrix.h"
#include "MantidMatrixDxExtensionHandler.h"
#include "MantidMatrixModel.h"
#include "MantidMatrixNullExtensionHandler.h"

MantidMatrixExtensionRequest::MantidMatrixExtensionRequest()
    : m_extensionHandler(new MantidMatrixNullExtensionHandler()) {}

MantidMatrixExtensionRequest::~MantidMatrixExtensionRequest() {}

/**
 * Create a MantidMatrix Tab Extension
 * @param type: provide the type
 * @returns a MantidMatrixTabeExtension
 */
MantidMatrixTabExtension
MantidMatrixExtensionRequest::createMantidMatrixTabExtension(
    MantidMatrixModel::Type type) {
  MantidMatrixTabExtension extension;

  switch (type) {
  case MantidMatrixModel::DX: {
    extension.label = "X Errors";
    extension.type = type;
    // Extend the chain of responsibility
    auto dxHandler =
        std::make_unique<MantidMatrixDxExtensionHandler>();
    dxHandler->setSuccessor(m_extensionHandler);
    m_extensionHandler = std::move(dxHandler);
    return extension;
  }
  default:
    throw std::runtime_error(
        "The requested extension type has not been implemented yet");
  }
}

/**
 * Set the number format for the selection type in the extensions
 * @param type: the model type
 * @param extensions: the extensions
 * @param format: the format
 * @param precision: the precision
 */
void MantidMatrixExtensionRequest::setNumberFormat(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    const QChar &format, int precision) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    m_extensionHandler->setNumberFormat(extension, format, precision);
    m_extensionHandler->recordFormat(extension, format, precision);
  } else {
    g_log.warning(
        "MantidMatrixExtensionRequest: Trying to alter an unknown extension.");
  }
}

/**
 * Set number format for all extensions
 * @param extensions: the extensions
 * @param format: the format
 * @param precision: the precision
 */
void MantidMatrixExtensionRequest::setNumberFormatForAll(
    MantidMatrixTabExtensionMap &extensions, const QChar &format,
    int precision) {
  for (auto it = extensions.begin(); it != extensions.end(); ++it) {
    auto &extension = it->second;
    m_extensionHandler->setNumberFormat(extension, format, precision);
  }
}

/**
 * Record the numeric format
 * @param type: the type which requires recording
 * @param extensions: the extensions
 * @param format: the format
 *@param precision: the precision
 */
void MantidMatrixExtensionRequest::recordFormat(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    const QChar &format, int precision) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    m_extensionHandler->recordFormat(extension, format, precision);
  } else {
    g_log.warning(
        "MantidMatrixExtensionRequest: Trying to alter an unknown extension.");
  }
}

/**
 * Get the format for the requested type
 * @param type: the type
 * @param extensions: the extensions
 * @param defaultValue: the default value
 * @returns the format character
 */
QChar MantidMatrixExtensionRequest::getFormat(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    QChar defaultValue) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    return m_extensionHandler->getFormat(extension);
  } else {
    g_log.warning(
        "MantidMatrixExtensionRequest: Trying to alter an unknown extension.");
    return defaultValue;
  }
}

/**
 * Get the preicson for the requested type
 * @param type: the type
 * @param extensions: the extensions
 * @param defaultValue: a default value
 * @returns the precision
 */
int MantidMatrixExtensionRequest::getPrecision(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    int defaultValue) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    return m_extensionHandler->getPrecision(extension);
  } else {
    g_log.warning(
        "MantidMatrixExtensionRequest: Trying to alter an unknown extension.");
    return defaultValue;
  }
}

/**
 * Set the column width
 * @param extensions: the extensions
 * @param width: the width
 * @param numberOfColumns: the number of columns
 */
void MantidMatrixExtensionRequest::setColumnWidthForAll(
    MantidMatrixTabExtensionMap &extensions, int width, int numberOfColumns) {
  for (auto it = extensions.begin(); it != extensions.end(); ++it) {
    auto &extension = it->second;
    m_extensionHandler->setColumnWidth(extension, width, numberOfColumns);
  }
}

/**
 * Get the table view for a specified type
 * @param type: the type
 * @param extensions: the extensions
 * @param width: the width
 * @param defaultValue: a default table view
 * @returns a table view object
 */
QTableView *MantidMatrixExtensionRequest::getTableView(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    int width, QTableView *defaultValue) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    m_extensionHandler->setColumnWidthPreference(extension, width);
    return m_extensionHandler->getTableView(extension);
  } else {
    return defaultValue;
  }
}

/**
 * Get the table view for a specified type
 * @param type: the type
 * @param extensions: the extensions
 * @param width: the width to set
 * @returns a table view object
 */
void MantidMatrixExtensionRequest::setColumnWidthPreference(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    int width) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    m_extensionHandler->setColumnWidthPreference(extension, width);
  } else {
    g_log.warning(
        "MantidMatrixExtensionRequest: Trying to alter an unknown extension.");
  }
}

/**
 * Get the column width
 * @param type: the type
 * @param extensions: the extensions
 * @param defaultValue: a default column width
 * @returns the precision
 */
int MantidMatrixExtensionRequest::getColumnWidth(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    int defaultValue) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    return m_extensionHandler->getColumnWidth(extension);
  } else {
    g_log.warning(
        "MantidMatrixExtensionRequest: Trying to alter an unknown extension.");
    return defaultValue;
  }
}

/**
 * Check if the table view matches the object
 * @param extensions: the extensions
 * @param object: the object to compare to
 * @returns true if an extension matches the object else false
 */
bool MantidMatrixExtensionRequest::tableViewMatchesObject(
    MantidMatrixTabExtensionMap &extensions, QObject *object) {
  for (auto it = extensions.begin(); it != extensions.end(); ++it) {
    auto &extension = it->second;
    if (extension.tableView.get() == object) {
      return true;
    }
  }
  return false;
}

/**
 * Get the requested active table view
 * @param type: the type
 * @param extensions: the extensions
 * @param defaultValue: a default table view pointer
 * @returns the active table view
 */
QTableView *MantidMatrixExtensionRequest::getActiveView(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    QTableView *defaultValue) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    return extension.tableView.get();
  } else {
    return defaultValue;
  }
}

/**
 * Get the requested active model
 * @param type: the type
 * @param extensions: the extensions
 * @param defaultValue: a default active model
 * @returns the active model
 */
MantidMatrixModel *MantidMatrixExtensionRequest::getActiveModel(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    MantidMatrixModel *defaultValue) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    return extension.model;
  } else {
    return defaultValue;
  }
}

/**
 * Get the column width preference
 * @param type: the type
 * @param extensions: the extensions
 * @param defaultValue: the default value
 * @returns the column width from the preferences
 */
int MantidMatrixExtensionRequest::getColumnWidthPreference(
    MantidMatrixModel::Type type, MantidMatrixTabExtensionMap &extensions,
    int defaultValue) {
  if (extensions.count(type) > 0) {
    auto &extension = extensions[type];
    return m_extensionHandler->getColumnWidthPreference(extension);
  } else {
    return defaultValue;
  }
}
