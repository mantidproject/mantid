// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QFileDialog>
#ifdef Q_OS_DARWIN
#include <errno.h>
#include <sys/sysctl.h>
#endif

namespace Mantid {
namespace Kernel {
class Property;
}
} // namespace Mantid

namespace MantidQt {
namespace API {
/**
    Contains modifications to Qt functions where problems have been found
    on certain operating systems
*/
namespace FileDialogHandler {
/**
 * @param parent :: the dialog will be shown centered over this parent
 * widget
 * @param baseProp :: The property that the dialog parameters will be extracted
 * from.
 * @param options :: The options argument holds various options about how
 * to run the dialog
 */
EXPORT_OPT_MANTIDQT_COMMON QString getSaveFileName(QWidget *parent = nullptr,
                                                   const Mantid::Kernel::Property *baseProp = nullptr,
                                                   const QFileDialog::Options &options = QFileDialog::Options());

/**
 * For file dialogs. This will add the selected extension if an extension
 * doesn't already exist.
 */
EXPORT_OPT_MANTIDQT_COMMON QString addExtension(const QString &filename, const QString &selectedFilter);

EXPORT_OPT_MANTIDQT_COMMON QString getFilter(const Mantid::Kernel::Property *baseProp);

/// For file dialogs
EXPORT_OPT_MANTIDQT_COMMON QString getFilter(const std::vector<std::string> &exts);

/// Format extension into expected form (*.ext)
EXPORT_OPT_MANTIDQT_COMMON QString formatExtension(const std::string &extension);

EXPORT_OPT_MANTIDQT_COMMON QString getCaption(const std::string &dialogName, const Mantid::Kernel::Property *prop);
} // namespace FileDialogHandler
} // namespace API
} // namespace MantidQt
