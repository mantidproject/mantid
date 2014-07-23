#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTCONVERTTOENERGY_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTCONVERTTOENERGY_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  /** IndirectConvertToEnergy

    @author Dan Nixon
    @date 23/07/2014

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport IndirectConvertToEnergy : public IndirectDataReductionTab
  {
    Q_OBJECT

  public:
    IndirectConvertToEnergy(Ui::IndirectDataReduction& uiForm, QWidget * parent = 0);
    virtual ~IndirectConvertToEnergy();

    virtual void setup();
    virtual void run();
    virtual bool validate();
  };
} // namespace CustomInterfaces
} // namespace Mantid

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTCONVERTTOENERGY_H_
