#ifndef MANTID_MDALGORITHMS_ACCUMULATEMD_H_
#define MANTID_MDALGORITHMS_ACCUMULATEMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace {}

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}

namespace MDAlgorithms {

std::vector<std::string>
filterToExistingSources(std::vector<std::string> input_data,
                        Kernel::Logger &g_log);

std::vector<std::string> filterToNew(std::vector<std::string> input_data,
                                     Kernel::Logger &g_log);

bool fileExists(const std::string &filename);

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
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_ACCUMULATEMD_H_ */
