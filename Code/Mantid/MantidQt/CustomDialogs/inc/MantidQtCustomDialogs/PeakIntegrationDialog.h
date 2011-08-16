#ifndef MANTIDQT_CUSTOM_DIALOGSPEAKINTEGRATIONDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSPEAKINTEGRATIONDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtAPI/AlgorithmDialog.h"

//---------------------------
// Qt Forward declarations
//---------------------------
class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QVBoxLayout;
class QCheckBox;

namespace MantidQt
{
namespace CustomDialogs
{

/** 
    This class gives specialised dialog for the PeakIntegration algorithm.

    @author Vickie Lynch, SNS
    @date 08/05/2011

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
class PeakIntegrationDialog : public MantidQt::API::AlgorithmDialog
{
  Q_OBJECT

public:

  /// Constructor
  PeakIntegrationDialog(QWidget *parent = 0);
  /// Destruktor
  ~PeakIntegrationDialog();

protected:
  /// This does the work and must be overridden in each deriving class
  void initLayout();
  void createLayout(int state);

protected slots:
  void createDynamicLayout(int state);

private:
  /* GUI components */
  QComboBox *wksp1_opt;
  QComboBox *wksp2_opt;
  QLineEdit *input;
  QLabel *label;
  QLineEdit *input1;
  QLabel *label1;
  QLineEdit *input2;
  QLabel *label2;
  QLineEdit *input3;
  QLabel *label3;
  QLineEdit *input4;
  QLabel *label4;
  QLineEdit *input5;
  QLabel *label5;
  QLineEdit *input6;
  QLabel *label6;
  QCheckBox *checkbox;
  QGridLayout * m_loaderLayout;
  QGridLayout * m_extrasLayout;
  QVBoxLayout * m_dialogLayout;

};

}
}

#endif /* MANTIDQT_CUSTOM_DIALOGSPEAKINTEGRATIONDIALOG_H */
