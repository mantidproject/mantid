#ifndef MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "ui_PlotAsymmetryByLogValueDialog.h"

#include <QString>
#include <QSignalMapper>

//---------------------------
// Qt Forward declarations
//---------------------------
class QVBoxLayout;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace MantidQt
{
namespace CustomDialogs
{

/** 
    This class gives specialised dialog for the LoadRaw algorithm.

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class PlotAsymmetryByLogValueDialog : public MantidQt::API::AlgorithmDialog
{

  Q_OBJECT
	
public:

  /// Constructor
  PlotAsymmetryByLogValueDialog(QWidget *parent = 0);
  ///Destructor
  ~PlotAsymmetryByLogValueDialog();

private:

  /** @name Virtual functions. */
  //@{
  /// Create the layout
  void initLayout();
  //@}
	
private slots:

  /// Opens a file dialog. Updates the QLineEdit provided when the dialog is closed.
  void openFileDialog(const QString& filePropName);
  void fillLogBox(const QString&);

  /// Show or hide Dead Time file widget depending on which Dead Time type is selected.
  void showHideDeadTimeFileWidget(int deadTimeTypeIndex);

private:

  // The form generated with Qt Designer
  Ui::PlotAsymmetryByLogValueDialog m_uiForm;

  /// Maps Browse buttons to file properties
  QSignalMapper* browseButtonMapper;
};

}
}

#endif //MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H
