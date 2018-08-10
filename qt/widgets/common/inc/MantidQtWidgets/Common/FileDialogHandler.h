#ifndef MANTIDQT_API_FILEDIALOGHANDLER_H_
#define MANTIDQT_API_FILEDIALOGHANDLER_H_

#include "MantidKernel/DllConfig.h"
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
namespace FileDialogHandler {
/**
 * @param parent :: the dialog will be shown centered over this parent
 * widget
 * @param baseProp :: The property that the dialog parameters will be extracted
 * from.
 * @param options :: The options argument holds various options about how
 * to run the dialog
 */
DLLExport QString
getSaveFileName(QWidget *parent = nullptr,
                const Mantid::Kernel::Property *baseProp = nullptr,
                QFileDialog::Options options = nullptr);

/**
 * For file dialogs. This will add the selected extension if an extension
 * doesn't
 * already exist.
 */
DLLExport QString addExtension(const QString &filename,
                               const QString &selectedFilter);

DLLExport QString getFilter(const Mantid::Kernel::Property *baseProp);

/** For file dialogs
 *
 * @param exts :: vector of extensions
 * @param defaultExt :: default extension to use
 * @return a string that filters files by extenstions
 */
DLLExport QString getFilter(const std::vector<std::string> &exts,
                            const std::string &defaultExt);

DLLExport QString getCaption(const std::string &dialogName,
                             const Mantid::Kernel::Property *prop);
} // namespace FileDialogHandler
} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_FILEDIALOGHANDLER_H_
