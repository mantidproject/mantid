// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FileDialogHandler.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include <boost/regex.hpp>
#include <sstream>

namespace { // anonymous namespace
const boost::regex FILE_EXT_REG_EXP{R"(^.+\s+\((\S+)\)$)"};
const QString ALL_FILES("All Files (*)");

QString getExtensionFromFilter(const QString &selectedFilter) {
  // empty returns empty
  if (selectedFilter.isEmpty()) {
    return QString("");
  }

  // search for single extension
  boost::smatch result;
  if (boost::regex_search(selectedFilter.toStdString(), result,
                          FILE_EXT_REG_EXP) &&
      result.size() == 2) {
    // clang fails to cast result[1] to std::string.
    std::string output = result[1];
    auto extension = QString::fromStdString(output);
    if (extension.startsWith("*"))
      return extension.remove(0, 1);
    else
      return extension;
  } else {
    // failure to match suggests multi-extension filter
    std::stringstream msg;
    msg << "Failed to determine single extension from \""
        << selectedFilter.toStdString() << "\"";
    throw std::runtime_error(msg.str());
  }
}
} // anonymous namespace

namespace MantidQt {
namespace API {
namespace FileDialogHandler {
/**
    Contains modifications to Qt functions where problems have been found
    on certain operating systems
*/
QString getSaveFileName(QWidget *parent,
                        const Mantid::Kernel::Property *baseProp,
                        QFileDialog::Options options) {
  // set up filters and dialog title
  const auto filter = getFilter(baseProp);
  const auto caption = getCaption("Save file", baseProp);

  QString selectedFilter;

  // create the file browser
  QString filename = QFileDialog::getSaveFileName(
      parent, caption, AlgorithmInputHistory::Instance().getPreviousDirectory(),
      filter, &selectedFilter, options);

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
  const auto *multiProp =
      dynamic_cast<const Mantid::API::MultipleFileProperty *>(baseProp);
  if (bool(multiProp))
    return getFilter(multiProp->getExts());

  // regular file version
  const auto *singleProp =
      dynamic_cast<const Mantid::API::FileProperty *>(baseProp);
  // The allowed values in this context are file extensions
  if (bool(singleProp))
    return getFilter(singleProp->allowedValues());

  // otherwise only the all files exists
  return ALL_FILES;
}

/** For file dialogs. Have each filter on a separate line with the default as
 * the first.
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
      for (auto &itr : exts) {
        // Add a space to between each extension
        displayAllFilter.append(" ");
        displayAllFilter.append(formatExtension(itr));
      }
      displayAllFilter.append(" );;");
      filter.append(displayAllFilter);
    }

    // Append individual file filters
    for (auto &itr : exts) {
      filter.append(QString::fromStdString(itr) + " (*" +
                    QString::fromStdString(itr) + ");;");
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

QString getCaption(const std::string &dialogName,
                   const Mantid::Kernel::Property *prop) {
  // generate the dialog title
  auto dialogTitle = QString::fromStdString(dialogName);
  if (bool(prop)) {
    const auto &name = prop->name();
    if (name != "Filename" && prop->name() != "Directory" &&
        prop->name() != "Dir") {
      dialogTitle.append(" - ");
      dialogTitle.append(QString::fromStdString(name));
    }
  }
  return dialogTitle;
}
} // namespace FileDialogHandler
} // namespace API
} // namespace MantidQt
