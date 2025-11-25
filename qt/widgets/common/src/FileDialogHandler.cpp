// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FileDialogHandler.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include <boost/regex.hpp>
#include <sstream>

namespace { // anonymous namespace
const QString ALL_FILES("All Files (*)");

QString getExtensionFromFilter(const QString &selectedFilter) {
  QString extension;
  // search for single extension
  static const boost::regex FILE_EXT_REG_EXP{R"(\*\.[[:word:]]+)"};
  boost::smatch result;
  const auto filter = selectedFilter.toStdString();
  if (boost::regex_search(filter, result, FILE_EXT_REG_EXP)) {
    // clang fails to cast result[1] to std::string.
    const std::string output = result.str(0);
    extension = QString::fromStdString(output);
    if (extension.startsWith("*"))
      extension.remove(0, 1);
  }
  return extension;
}

} // anonymous namespace

namespace MantidQt::API::FileDialogHandler {
/**
    Contains modifications to Qt functions where problems have been found
    on certain operating systems
*/
QString getSaveFileName(QWidget *parent, const Mantid::Kernel::Property *baseProp,
                        const QFileDialog::Options &options) {
  // set up filters and dialog title
  const auto filter = getFilter(baseProp);
  const auto caption = getCaption("Save file", baseProp);

  QString selectedFilter;

  // create the file browser
  const QString filename = QFileDialog::getSaveFileName(
      parent, caption, AlgorithmInputHistory::Instance().getPreviousDirectory(), filter, &selectedFilter, options);
  return addExtension(filename, selectedFilter);
}

QString addExtension(const QString &filename, const QString &selectedFilter) {
  // just return an empty string if that is what was given
  if (filename.isEmpty())
    return filename;

  // Check the filename and append the selected filter if necessary
  if (QFileInfo(filename).completeSuffix().isEmpty()) {
    auto ext = getExtensionFromFilter(selectedFilter);
    if (filename.endsWith(".") && ext.startsWith(".")) {
      ext = ext.remove(0, 1);
    }
    return filename + ext;
  } else {
    return filename;
  }
}

QString getFilter(const Mantid::Kernel::Property *baseProp) {
  if (!baseProp)
    return ALL_FILES;

  // multiple file version
  const auto *multiProp = dynamic_cast<const Mantid::API::MultipleFileProperty *>(baseProp);
  if (multiProp)
    return getFilter(multiProp->getExts());

  // regular file version
  const auto *singleProp = dynamic_cast<const Mantid::API::FileProperty *>(baseProp);
  // The allowed values in this context are file extensions
  if (singleProp)
    return getFilter(singleProp->allowedValues());

  // otherwise only the all files exists
  return ALL_FILES;
}

/** For file dialogs. Have each filter on a separate line with Data Files
 * as the first and All Files as the last
 *
 * @param exts :: vector of extensions
 * @return a string that filters files by extenstions
 */
QString getFilter(const std::vector<std::string> &exts) {
  QString filter("");

  if (!exts.empty()) {
    // Generate the display all filter
    if (exts.size() > 1) {
      QString displayAllFilter = "Data Files (";
      for (auto const &itr : exts) {
        // Add a space to between each extension
        displayAllFilter.append(" ");
        displayAllFilter.append(formatExtension(itr));
      }
      displayAllFilter.append(" );;");
      filter.append(displayAllFilter);
    }

    // Append individual file filters
    for (auto const &itr : exts) {
      filter.append(QString::fromStdString(itr) + " (*" + QString::fromStdString(itr) + ");;");
    }
    filter = filter.trimmed();
  }
  filter.append(ALL_FILES);
  return filter;
}

/** Format extension into expected form (*.ext)
 *
 * @param extension :: extension to be formatted
 * @return a QString of the expected form
 */
QString formatExtension(const std::string &extension) {
  QString formattedExtension = QString::fromStdString(extension);
  if (extension.empty()) {
    return formattedExtension;
  }
  if (extension.at(0) == '*' && extension.at(1) == '.') {
    return formattedExtension;
  } else {
    if (extension.at(0) == '*') {
      formattedExtension.insert(1, ".");
    } else if (extension.at(0) == '.') {
      formattedExtension.prepend("*");
    } else {
      formattedExtension.prepend("*.");
    }
  }
  return formattedExtension;
}

QString getCaption(const std::string &dialogName, const Mantid::Kernel::Property *prop) {
  // generate the dialog title
  auto dialogTitle = QString::fromStdString(dialogName);
  if (bool(prop)) {
    const auto &name = prop->name();
    if (name != "Filename" && prop->name() != "Directory" && prop->name() != "Dir") {
      dialogTitle.append(" - ");
      dialogTitle.append(QString::fromStdString(name));
    }
  }
  return dialogTitle;
}
} // namespace MantidQt::API::FileDialogHandler
