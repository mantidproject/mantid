#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADER_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {
/// Ways to deal with dead time correction
enum class DeadTimesType { None, FromFile, FromDisk };

/// Data loaded from file
struct LoadResult {
  Mantid::API::Workspace_sptr loadedWorkspace;
  Mantid::API::Workspace_sptr loadedGrouping;
  Mantid::API::Workspace_sptr loadedDeadTimes;
  std::string mainFieldDirection;
  double timeZero;
  double firstGoodData;
  std::string label;
};
}
}
}

namespace MantidQt {
namespace CustomInterfaces {

/** MuonAnalysisDataLoader : Loads and processes muon data for MuonAnalysis

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
class MANTIDQT_CUSTOMINTERFACES_DLL MuonAnalysisDataLoader {
public:
  /// constructor
  MuonAnalysisDataLoader(const Muon::DeadTimesType &deadTimesType,
                         const QStringList &instruments,
                         const std::string &deadTimesFile = "");
  /// change dead times type
  void setDeadTimesType(const Muon::DeadTimesType &deadTimesType,
                        const std::string &deadTimesFile = "");
  /// set available instruments
  /// load files
  Muon::LoadResult loadFiles(const QStringList &files) const;

private:
  /// Get instrument name from workspace
  std::string
  getInstrumentName(const Mantid::API::Workspace_sptr workspace) const;
  /// Dead times type
  Muon::DeadTimesType m_deadTimesType;
  /// Dead times file
  std::string m_deadTimesFile;
  /// Muon instruments supported
  const QStringList m_instruments;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQt_CUSTOMINTERFACES_MUONANALYSISDATALOADER_H_ */