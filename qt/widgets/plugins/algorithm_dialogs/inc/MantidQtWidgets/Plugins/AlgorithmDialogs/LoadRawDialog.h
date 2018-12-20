#ifndef MANTIDQT_CUSTOM_DIALOGSLOADRAWDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSLOADRAWDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"

#include <QString>

//---------------------------
// Qt Forward declarations
//---------------------------
class QVBoxLayout;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace MantidQt {
namespace CustomDialogs {

/**
    This class gives specialised dialog for the LoadRaw algorithm.

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009

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
class LoadRawDialog : public MantidQt::API::AlgorithmDialog {

  Q_OBJECT

public:
  /// Constructor
  LoadRawDialog(QWidget *parent = nullptr);
  /// Destructor
  ~LoadRawDialog() override;

private:
  /** @name Virtual functions. */
  //@{
  /// Create the layout
  void initLayout() override;
  //@}

private slots:

  /// A slot for the browse button clicked signal
  void browseClicked();

private:
  /// The line inputs
  QLineEdit *m_pathBox, *m_wsBox;
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMDIALOGS_LOADRAWDIALOG_H
