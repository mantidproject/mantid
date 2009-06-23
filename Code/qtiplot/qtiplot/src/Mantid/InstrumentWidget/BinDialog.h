#ifndef BINDIALOG_H_
#define BINDIALOG_H_

#include <QDialog>

// Qt forward declarations
class QLineEdit;
class QRadioButton;

/*!
  \class  BinDialog
  \brief  class to display Bin selection dialog
  \author Srikanth Nagella
  \date   November 2008
  \version 1.0

  BinDialog class handles the Input Dialog for Bin selection and operation on it.

  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
*/


class BinDialog: public QDialog
{
    Q_OBJECT
public:
    BinDialog(QWidget *parent = 0);
    ~BinDialog();
	void setIntegralMinMax(double,double);
signals:
	void IntegralMinMax(double,double);
public slots:
	void btnOKClicked();
private:
	QLineEdit* mIntegralMinValue;
	QLineEdit* mIntegralMaxValue;

};

#endif /*BINDIALOG_H_*/

