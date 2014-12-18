#ifndef MULTIFITSETUPTDIALOG_H
#define MULTIFITSETUPTDIALOG_H

#include "ui_MultifitSetupDialog.h"
#include <QDialog>

namespace MantidQt
{
namespace MantidWidgets
{
  class FitPropertyBrowser;
/** 
    This is a dialog for doing setting up the MultiBG function.
    

    @author Roman Tolchenov, Tessella plc
    @date 7/09/2011

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
class MultifitSetupDialog : public QDialog
{
  Q_OBJECT

public:
  
  /// Default constructor
  MultifitSetupDialog(FitPropertyBrowser* fitBrowser);

  /// Returns a list of parameter ties. Empty string means no ties and parameter is local
  QStringList getParameterTies()const{return m_ties;}

private slots:

  /// Setup the function and close dialog
  void accept();
  void cellChanged(int,int);

private:

  /// The form generated with Qt Designer
  Ui::MultifitSetupDialog ui;

  /// Pointer to the calling fit browser
  FitPropertyBrowser* m_fitBrowser;

  /// A list with parameter ties
  QStringList m_ties;

};


}
}

#endif /* MULTIFITSETUPTDIALOG_H */
