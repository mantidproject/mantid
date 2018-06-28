/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
#define MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
#include "DllConfig.h"
#include <memory>
#include <vector>
#include <string>
#include "RunsTablePresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RunsTablePresenterFactory {
public:
  RunsTablePresenterFactory(std::vector<std::string> const &instruments,
                            double thetaTolerance,
                            WorkspaceNamesFactory const &workspaceNamesFactory);
  std::unique_ptr<RunsTablePresenter> operator()(IRunsTableView *view) const;

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  WorkspaceNamesFactory m_workspaceNamesFactory;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
