#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include <boost/regex.hpp>

namespace { // anonymous namespace
const boost::regex FILE_EXT_REG_EXP{"^.+\\s+\\(\\*(\\S+).+"};

QString getExtension(const QString &selectedFilter) {
  if (selectedFilter.isEmpty()) {
    return QString("");
  }

  boost::smatch result;
  if (boost::regex_search(selectedFilter.toStdString(), result,
                          FILE_EXT_REG_EXP) &&
      result.size() == 2) {
    return QString::fromStdString(result[1]);
  } else {
    // failure returns empty string
    return QString("");
  }
}
} // anonymous namespace

namespace MantidQt {
namespace API {
namespace FileDialogHandler {
/**
    Contains modifications to Qt functions where problems have been found
    on certain operating systems

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source
    @date 17/09/2010

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
QString getSaveFileName(QWidget *parent,
                        const Mantid::Kernel::Property *baseProp,
                        QFileDialog::Options options) {
  // set up filters
  const auto filter = getFileDialogFilter(baseProp);

  // generate the dialog title
  QString dialogTitle("Save file");
  if (bool(baseProp) && baseProp->name() != "Filename") {
    dialogTitle.append(" - ");
    dialogTitle.append(QString::fromStdString(baseProp->name()));
  }

  QString selectedFilter;

  // create the file browser
  QString filename = QFileDialog::getSaveFileName(
      parent, dialogTitle,
      AlgorithmInputHistory::Instance().getPreviousDirectory(), filter,
      &selectedFilter, options);

  return addExtension(filename, selectedFilter);
}

QString addExtension(const QString &filename, const QString &selectedFilter) {
  // just return an empty string if that is what was given
  if (filename.isEmpty())
    return filename;

  // Check the filename and append the selected filter if necessary
  if (QFileInfo(filename).completeSuffix().isEmpty()) {
    auto ext = getExtension(selectedFilter);
    if (filename.endsWith(".") && ext.startsWith(".")) {
      ext = ext.remove(0, 1);
    }
    return filename + ext;
  } else {
    return filename;
  }
}

QString getFileDialogFilter(const Mantid::Kernel::Property *baseProp) {
  if (!baseProp)
    return QString("All Files (*)");

  // multiple file version
  const auto *multiProp =
      dynamic_cast<const Mantid::API::MultipleFileProperty *>(baseProp);
  if (bool(multiProp))
    return getFileDialogFilter(multiProp->getExts(),
                               multiProp->getDefaultExt());

  // regular file version
  const auto *singleProp =
      dynamic_cast<const Mantid::API::FileProperty *>(baseProp);
  // The allowed values in this context are file extensions
  if (bool(singleProp))
    return getFileDialogFilter(singleProp->allowedValues(),
                               singleProp->getDefaultExt());

  // otherwise only the all files exists
  return QString("All Files (*)");
}

/** For file dialogs. Have each filter on a separate line with the default as
 * the first.
 *
 * @param exts :: vector of extensions
 * @param defaultExt :: default extension to use
 * @return a string that filters files by extenstions
 */
QString getFileDialogFilter(const std::vector<std::string> &exts,
                            const std::string &defaultExt) {
  QString filter("");

  if (!defaultExt.empty()) {
    filter.append(QString::fromStdString(defaultExt) + " (*" +
                  QString::fromStdString(defaultExt) + ");;");
  }

  if (!exts.empty()) {
    // --------- Load a File -------------
    auto iend = exts.end();
    // Push a wild-card onto the front of each file suffix
    for (auto itr = exts.begin(); itr != iend; ++itr) {
      if ((*itr) != defaultExt) {
        filter.append(QString::fromStdString(*itr) + " (*" +
                      QString::fromStdString(*itr) + ");;");
      }
    }
    filter = filter.trimmed();
  }
  filter.append("All Files (*)");
  return filter;
}
}
}
}
