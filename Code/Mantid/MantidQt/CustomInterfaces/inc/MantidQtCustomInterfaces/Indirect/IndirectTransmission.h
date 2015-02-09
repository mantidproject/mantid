#ifndef MANTID_CUSTOMINTERFACES_INDIRECTTRANSMISSION_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTTRANSMISSION_H_

#include "IndirectDataReductionTab.h"
#include "ui_IndirectTransmission.h"
#include "MantidKernel/System.h"


namespace MantidQt
{
namespace CustomInterfaces
{
  /** IndirectTransmission

    Provides the UI interface to the IndirectTransmissionMonitor algorithm to calculate
    sample transmission using a sample and container raw run file.


    @author Samuel Jackson
    @date 13/08/2013

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
  class DLLExport IndirectTransmission : public IndirectDataReductionTab
  {
    Q_OBJECT

  public:
    IndirectTransmission(IndirectDataReduction * idrUI, QWidget * parent = 0);
    virtual ~IndirectTransmission();

    virtual void setup();
    virtual void run();
    virtual bool validate();

  private slots:
    void dataLoaded();
    void previewPlot();
    void transAlgDone(bool error);

  private:
    Ui::IndirectTransmission m_uiForm;

  };

} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_INDIRECTTRANSMISSION_H_ */
