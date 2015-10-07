#ifndef MANTIDQT_API_GENERICDIALOG_H_
#define MANTIDQT_API_GENERICDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "AlgorithmDialog.h"

#include <QHash>
#include <QtCore/qvariant.h>
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Property.h"
#include "MantidQtAPI/AlgorithmPropertiesWidget.h"

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
class GenericDialog : public AlgorithmDialog
{
  
  Q_OBJECT

public:

  // Constructor
  GenericDialog(QWidget* parent = 0);
  // Destructor
  virtual ~GenericDialog();

protected slots:
  virtual void accept();

private:
  virtual void initLayout();

  void parseInput();

  /// Widget containing all the PropertyWidgets
  AlgorithmPropertiesWidget * m_algoPropertiesWidget;

};

}
}

#endif //MANTIDQT_API_GENERICDIALOG_H_
