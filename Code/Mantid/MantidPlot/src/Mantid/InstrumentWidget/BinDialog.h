#ifndef BINDIALOG_H_
#define BINDIALOG_H_

#include <QDialog>
#include <QCheckBox>

// Qt forward declarations
class QLineEdit;
class QRadioButton;

/**
  \class  BinDialog
  \brief  class to display Bin selection dialog
  \author Srikanth Nagella
  \date   November 2008
  \version 1.0

  BinDialog class handles the Input Dialog for bin selection:
  e.g. enter bin range from X1 to X2.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
 */


class BinDialog: public QDialog
{
  Q_OBJECT

public:
  BinDialog(QWidget *parent = 0);
  ~BinDialog();
  void setIntegralMinMax(double,double,bool);
  signals:
  /// This signal is sent when changing the bin range selected.
  /// Parameters are: min, max, and a bool set to true to mean "everything"
  void IntegralMinMax(double,double,bool);

public slots:
  void btnOKClicked();
  void mEntireRange_toggled(bool on);

private:
  QLineEdit* mIntegralMinValue;
  QLineEdit* mIntegralMaxValue;
  QCheckBox* mEntireRange;

};

#endif /*BINDIALOG_H_*/

