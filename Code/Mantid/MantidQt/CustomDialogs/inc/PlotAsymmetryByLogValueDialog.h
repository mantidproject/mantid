#ifndef MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "ui_PlotAsymmetryByLogValueDialog.h"

#include <QString>

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
  void parseInput();
  //@}
	
private slots:

  /// A slot for the browse button clicked signal
  void browseFirstClicked();
  void browseLastClicked();
  void fillLogBox(const QString&);

private:


  // The form generated with Qt Designer
  Ui::PlotAsymmetryByLogValueDialog m_uiForm;
};

}
}

#endif //MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H
