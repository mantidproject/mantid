#ifndef MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_INPUTDATACONTROL_H_
#define MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_INPUTDATACONTROL_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
// Mantid headers from other projects
#include "DllConfig.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
// 3rd party library headers
#include <QObject>
// system headers

// Class forward declarations
namespace MantidAPI {
class MatrixWorkspace;
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/** Class to handle all input regarding fitting with the
  BackgroundRemover

  @date 2016-03-15

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTIDQT_DYNAMICPDF_DLL InputDataControl
    : public QObject,
      public MantidQt::API::WorkspaceObserver {
  Q_OBJECT

public:
  InputDataControl();
  ~InputDataControl();
  std::vector<double> selectedDataX();
  std::vector<double> selectedDataY();
  std::vector<double> selectedDataE();
  double getSelectedEnergy();
  std::string getWorkspaceName();
  size_t getWorkspaceIndex();
  bool isSliceSelectedForFitting();
  std::pair<double, double> getCurrentRange();

protected:
  void preDeleteHandle(
      const std::string &workspaceName,
      const boost::shared_ptr<Mantid::API::Workspace> workspace) override;

signals:
  void signalWorkspaceUpdated();
  void signalSliceForFittingUpdated();

public slots:
  void updateWorkspace(const QString &workspaceName);
  void updateSliceForFitting(const size_t &workspaceIndex);

private:
  void updateDomain();
  /// workspace selected for fitting
  boost::shared_ptr<Mantid::API::MatrixWorkspace> m_workspace;
  /// current sliced to be displayed and/or fitted
  size_t m_selectedWorkspaceIndex;
  /// energy range for each slice with non-zero signal. We use the
  /// the indexes along the Energy range
  std::vector<std::pair<int, int>> m_domain;

}; // class InputDataControl
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_INPUTDATACONTROL_H_
