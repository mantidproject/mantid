#ifndef MANTID_MDALGORITHMS_ACCUMULATEMD_H_
#define MANTID_MDALGORITHMS_ACCUMULATEMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include <set>

namespace {}

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}

namespace MDAlgorithms {

void filterToExistingSources(std::vector<std::string> &input_data,
                             std::vector<double> &psi, std::vector<double> &gl,
                             std::vector<double> &gs,
                             std::vector<double> &efix);

bool dataExists(const std::string &data_name);

void filterToNew(std::vector<std::string> &input_data,
                 std::vector<std::string> &current_data,
                 std::vector<double> &psi, std::vector<double> &gl,
                 std::vector<double> &gs, std::vector<double> &efix);

bool appearsInCurrentData(const std::string &input_data,
                          std::vector<std::string> &current_data);

std::vector<std::string>
getHistoricalDataSources(const API::WorkspaceHistory &ws_history);

void insertDataSources(const std::string &data_sources,
                       std::set<std::string> &historical_data_sources);

bool fileExists(const std::string &filename);

void padParameterVector(std::vector<double> &param_vector);

/** AccumulateMD : Algorithm for appending new data to a MDHistoWorkspace

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AccumulateMD : public API::Algorithm {
public:
  AccumulateMD();
  virtual ~AccumulateMD();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();

  Mantid::API::IMDWorkspace_sptr createMDWorkspace(
      const std::vector<std::string> &data_sources,
      const std::vector<double> &psi, const std::vector<double> &gl,
      const std::vector<double> &gs, const std::vector<double> &efix);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_ACCUMULATEMD_H_ */
