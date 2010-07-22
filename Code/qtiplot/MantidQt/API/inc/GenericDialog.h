#ifndef MANTIDQT_API_GENERICDIALOG_H_
#define MANTIDQT_API_GENERICDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "AlgorithmDialog.h"

#include <QHash>

//----------------------------------
// Forward declarations
//----------------------------------
class QSignalMapper;
class QGridLayout;
class QLineEdit;

namespace MantidQt
{

namespace API
{

/** 
    This class gives a basic dialog that is not tailored to a particular 
    algorithm.

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
class GenericDialog : public AlgorithmDialog
{
  
  Q_OBJECT

public:

  // Constructor
  GenericDialog(QWidget* parent = 0);
  // Destructor
  virtual ~GenericDialog();

private:

  /// This does the work and must be overridden in each deriving class
  virtual void initLayout();

private slots:

  /// Browse for a file
  void browseClicked(QWidget* editBox);

private:

  /// A mapper to ensure that the  buttons are connected to the correct editing boxes
  QSignalMapper *m_signalMapper; 
  
  /// The grid widget containing the input boxes
  QGridLayout *m_inputGrid;

  /// A map of QLineEdit pointers to the Property names
  QHash<QLineEdit*, QString> m_editBoxes;
};

}
}

#endif //MANTIDQT_API_GENERICDIALOG_H_
