#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECT_H_

#include "ui_IndirectDataReduction.h"

#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"

#include "MantidQtMantidWidgets/RangeSelector.h"
#include "MantidAPI/MatrixWorkspace.h"

//----------------------------------------------------
// Forward declarations
//-----------------------------------------------------

class QtProperty;
class QtBoolPropertyManager;
class QtDoublePropertyManager;
class QtGroupPropertyManager;
class QtTreePropertyBrowser;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Forward Declarations
    class Background;

    /** 
    This class defines handles the ConvertToEnergy interface for indirect instruments (IRIS/OSIRIS).    

    @author Michael Whitty
    @author Martyn Gigg

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    class Indirect : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// explicit constructor, not to allow any implicit conversion of types
      explicit Indirect(QWidget *parent, Ui::IndirectDataReduction & uiForm);
    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
