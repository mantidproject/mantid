#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPER_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtMantidWidgets/IMuonFitDataSelector.h"
#include "MantidQtMantidWidgets/IWorkspaceFitControl.h"

namespace MantidQt {
namespace CustomInterfaces {

/** MuonAnalysisFitDataHelper : Updates fit browser from data widget

  When data widget reports changes, MuonAnalysis uses this helper class
  to update the fit browser. It is implemented as a separate class for
  testing purposes.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTIDQT_CUSTOMINTERFACES_DLL MuonAnalysisFitDataHelper {
public:
  /// Constructor
  MuonAnalysisFitDataHelper(
      MantidQt::MantidWidgets::IWorkspaceFitControl *fitBrowser,
      MantidQt::MantidWidgets::IMuonFitDataSelector *dataSelector);
  /// Handles "workspace properties changed"
  void handleWorkspacePropertiesChanged();
  /// Handles "selected groups changed"
  void handleSelectedGroupsChanged();
  /// Handles "selected periods changed"
  void handleSelectedPeriodsChanged();
  /// Handles user changing X range by dragging lines
  void handleXRangeChangedGraphically(double start, double end);
  /// Handles peak picker being reassigned to a new graph
  void peakPickerReassigned(const QString &wsName);

private:
  /// Fit browser to update (non-owning pointer)
  MantidQt::MantidWidgets::IWorkspaceFitControl *m_fitBrowser;
  /// Data selector to get input from (non-owning pointer)
  MantidQt::MantidWidgets::IMuonFitDataSelector *m_dataSelector;
};

} // namespace CustomInterfaces
} // namespace Mantid

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPER_H_ */