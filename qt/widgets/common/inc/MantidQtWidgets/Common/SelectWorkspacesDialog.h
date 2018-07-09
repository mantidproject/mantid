#ifndef SELECTWORKSPACESDIALOG_H
#define SELECTWORKSPACESDIALOG_H

//----------------------------
//   Includes
//----------------------------

#include "DllOption.h"
#include <QDialog>
#include <QListWidget>
#include <QStringList>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

/**
    This is a dialog for selecting workspaces.

    @author Roman Tolchenov, Tessella plc
    @date 22/06/2010

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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
class EXPORT_OPT_MANTIDQT_COMMON SelectWorkspacesDialog : public QDialog {
  Q_OBJECT

public:
  /// return value of the Custom button
  static const int CustomButton = 45654; // do not use this number direct, just
                                         // refer to this static constant

  /// Constructor
  SelectWorkspacesDialog(QWidget *parent = nullptr,
                         const std::string &typeFilter = "",
                         const std::string &customButtonLabel = "");

  /// Return the selected names
  QStringList getSelectedNames() const;

private slots:

  /// Slot to monitor the workspace selection status
  void selectionChanged();

  /// slot to handle the custom button press
  void customButtonPress();

private:
  /// Displays available workspace names
  QListWidget *m_wsList;
  /// The OK button
  QPushButton *m_okButton;
  /// The OK button
  QPushButton *m_customButton;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* SELECTWORKSPACESDIALOG_H */
