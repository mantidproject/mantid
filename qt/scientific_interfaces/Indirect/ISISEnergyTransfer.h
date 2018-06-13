#ifndef MANTIDQTCUSTOMINTERFACES_ISISENERGYTRANSFER_H_
#define MANTIDQTCUSTOMINTERFACES_ISISENERGYTRANSFER_H_

#include "IndirectDataReductionTab.h"
#include "ui_ISISEnergyTransfer.h"
#include "MantidKernel/System.h"
#include "../General/Background.h"

namespace MantidQt {
namespace CustomInterfaces {
/** ISISEnergyTransfer
  Handles an energy transfer reduction for ISIS instruments.

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
class DLLExport ISISEnergyTransfer : public IndirectDataReductionTab {
  Q_OBJECT

public:
  ISISEnergyTransfer(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~ISISEnergyTransfer() override;

  void setup() override;
  void run() override;

public slots:
  bool validate() override;

private slots:
  void algorithmComplete(bool error);
  void
  setInstrumentDefault(); ///< Sets default parameters for current instrument
  void mappingOptionSelected(
      const QString &groupType); ///< change ui to display appropriate options
  void plotRaw();                ///< plot raw data from instrument
  void
  pbRunEditing(); //< Called when a user starts to type / edit the runs to load.
  void pbRunFinding();  //< Called when the FileFinder starts finding the files.
  void pbRunFinished(); //< Called when the FileFinder has finished finding the
  // files.
  void plotRawComplete(
      bool error); //< Called when the Plot Raw algorithmm chain completes
  /// Handles plotting and saving
  void plotClicked();
  void saveClicked();

private:
  Ui::ISISEnergyTransfer m_uiForm;

  std::pair<std::string, std::string> createMapFile(
       const std::string &
          groupType); ///< create the mapping file with which to group results
  std::vector<std::string> getSaveFormats(); ///< get a vector of save formats
  std::vector<std::string>
      m_outputWorkspaces; ///< get a vector of workspaces to plot
  QString validateDetectorGrouping();
  const std::string createDetectorGroupingString();
};
} // namespace CustomInterfaces
} // namespace Mantid

#endif // MANTIDQTCUSTOMINTERFACES_ISISENERGYTRANSFER_H_
