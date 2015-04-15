#ifndef MANTIDQTCUSTOMINTERFACES_ILLCALIBRATION_H_
#define MANTIDQTCUSTOMINTERFACES_ILLCALIBRATION_H_

#include "IndirectDataReductionTab.h"
#include "ui_ILLCalibration.h"
#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  /** ILLCalibration

    @author Dan Nixon

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport ILLCalibration : public IndirectDataReductionTab
  {
    Q_OBJECT

  public:
    ILLCalibration(IndirectDataReduction * idrUI, QWidget * parent = 0);
    virtual ~ILLCalibration();

    virtual void setup();
    virtual void run();
    virtual bool validate();

  private slots:
    void algorithmComplete(bool error);
    void newInstrumentSelected();

  private:
    Ui::ILLCalibration m_uiForm;

  };

} // namespace CustomInterfaces
} // namespace Mantid

#endif //MANTIDQTCUSTOMINTERFACES_ILLCALIBRATION_H_
