#ifndef MANTIDQTCUSTOMINTERFACES_ILLENERGYTRANSFER_H_
#define MANTIDQTCUSTOMINTERFACES_ILLENERGYTRANSFER_H_

#include "IndirectDataReductionTab.h"
#include "ui_ILLEnergyTransfer.h"
#include "MantidKernel/System.h"

namespace MantidQt {
namespace CustomInterfaces {
/** ILLEnergyTransfer

  @author Dan Nixon
  @date 23/07/2014

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class DLLExport ILLEnergyTransfer : public IndirectDataReductionTab {
  Q_OBJECT

public:
  ILLEnergyTransfer(IndirectDataReduction *idrUI, QWidget *parent = 0);
  ~ILLEnergyTransfer() override;

  void setup() override;
  void run() override;

public slots:
  bool validate() override;

private slots:
  void algorithmComplete(bool error);
  void setInstrumentDefault();

private:
  Ui::ILLEnergyTransfer m_uiForm;
  double m_backScaling = 1.;
  double m_backCalibScaling = 1.;
  double m_peakRange[2];
  int m_pixelRange[2];
  std::string m_suffix;
  void save();
  void plot();
};
} // namespace CustomInterfaces
} // namespace Mantid

#endif // MANTIDQTCUSTOMINTERFACES_ILLENERGYTRANSFER_H_
