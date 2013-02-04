#ifndef MANTIDQT_API_FILEDIALOGHANDLER_H_
#define MANTIDQT_API_FILEDIALOGHANDLER_H_

#include <QFileDialog>
#ifdef Q_OS_DARWIN
  #include <errno.h>
  #include <sys/sysctl.h>
#endif


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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
struct FileDialogHandler
{
  /** The MacOS's native save dialog crashes when running a 10.6 package on 10.8 so this function, which takes
  *  the same arguments as the Qt function, ensures a nonnative object is used on the Mac when necessary.
  *  If compiled on 10.8 the native will be used
  *  @param parent :: the dialog will be shown centered over this parent widget
  *  @param caption :: The dialog's caption 
  *  @param dir :: The file dialog's working directory will be set to dir. If dir includes a file name, the file will be selected
  *  @param filter :: extensions of files to look for
  *  @param selectedFilter :: pass a pointer an existing string that will be filled with the extension the user selected
  *  @param options :: The options argument holds various options about how to run the dialog
  */
  static QString getSaveFileName(QWidget * parent = 0, const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString(), QString * selectedFilter = 0, QFileDialog::Options options = 0)
  {
#if defined(Q_OS_DARWIN) && defined(__MAC_OS_X_VERSION_MAX_ALLOWED)
  // If we are compiling on Snow Leopard
  #if __MAC_OS_X_VERSION_MAX_ALLOWED == 1060
    static int runningMountainLion(-1);
    if(runningMountainLion < 0)
    {
      // Check if we are running Mountain Lion
      // osrelease 12.x.x is OS X 10.8: http://stackoverflow.com/questions/11072804/mac-os-x-10-8-replacement-for-gestalt-for-testing-os-version-at-runtime
      char str[256];
      size_t size = sizeof(str);
      sysctlbyname("kern.osrelease", str, &size, NULL, 0);
      if(str[0] == '1' && str[1] == '2' ) runningMountainLion = 1;
      else runningMountainLion = 0;
    }
    if(runningMountainLion) options = options | QFileDialog::DontUseNativeDialog;
  #endif
#endif
    return QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
  }

};

}
}

#endif          //MANTIDQT_API_FILEDIALOGHANDLER_H_
