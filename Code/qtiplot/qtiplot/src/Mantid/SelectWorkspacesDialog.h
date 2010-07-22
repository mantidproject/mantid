#ifndef SELECTWORKSPACESDIALOG_H
#define SELECTWORKSPACESDIALOG_H

//----------------------------
//   Includes
//----------------------------

#include <QDialog>
#include <QListWidget>
#include <QStringList>

//----------------------------
//   Forward declarations
//----------------------------

class ApplicationWindow;

/** 
    This is a dialog for selecting workspaces.

    @author Roman Tolchenov, Tessella plc
    @date 22/06/2010

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class SelectWorkspacesDialog : public QDialog
{
  Q_OBJECT

public:
  
  /// Constructor
  SelectWorkspacesDialog (ApplicationWindow* appWindow);

  /// Return the selected names
  QStringList getSelectedNames()const;

private:

  /// Displays available workspace names
  QListWidget* m_wsList;

};



#endif /* SELECTWORKSPACESDIALOG_H */
