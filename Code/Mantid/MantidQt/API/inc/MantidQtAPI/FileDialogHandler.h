#ifndef MANTIDQT_API_FILEDIALOGHANDLER_H_
#define MANTIDQT_API_FILEDIALOGHANDLER_H_

#include <QFileDialog>

namespace MantidQt
{
namespace API 
{
/** 
    Contains modifications to Qt functions where problems have been found
    on certain operating systems

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
struct FileDialogHandler
{
  /** The MacOS's native save file browse button hangs so this function, which takes
  *  the same arguments as the Qt function, ensures a nonnative object is used on the Mac
  *  @param parent :: the dialog will be shown centered over this parent widget
  *  @param caption :: The dialog's caption 
  *  @param dir :: The file dialog's working directory will be set to dir. If dir includes a file name, the file will be selected
  *  @param filter :: extensions of files to look for
  *  @param selectedFilter :: pass a pointer an existing string that will be filled with the extension the user selected
  *  @param options :: The options argument holds various options about how to run the dialog
  */
  static QString getSaveFileName(QWidget * parent = 0, const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString(), QString * selectedFilter = 0, QFileDialog::Options options = 0)
  {
#ifdef Q_OS_DARWIN
    options = options | QFileDialog::DontUseNativeDialog;
#endif
    return QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
  }
};

}
}

#endif          //MANTIDQT_API_FILEDIALOGHANDLER_H_
