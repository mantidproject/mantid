#ifndef MANTID_DATAHANDLING_LOADDIFFCAL_H_
#define MANTID_DATAHANDLING_LOADDIFFCAL_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

namespace H5 {
class H5File;
class Group;
}

namespace Mantid {
namespace DataHandling {

/** LoadDiffCal : TODO: DESCRIPTION

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
class DLLExport LoadDiffCal : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SaveDiffCal"};
  }
  const std::string category() const override;
  const std::string summary() const override;

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  void init() override;
  void exec() override;
  void getInstrument(H5::H5File &file);
  void runLoadCalFile();
  void makeGroupingWorkspace(const std::vector<int32_t> &detids,
                             const std::vector<int32_t> &groups);
  void makeMaskWorkspace(const std::vector<int32_t> &detids,
                         const std::vector<int32_t> &use);
  void makeCalWorkspace(const std::vector<int32_t> &detids,
                        const std::vector<double> &difc,
                        const std::vector<double> &difa,
                        const std::vector<double> &tzero,
                        const std::vector<int32_t> &dasids,
                        const std::vector<double> &offsets,
                        const std::vector<int32_t> &use);

  std::string m_filename;
  std::string m_workspaceName;
  Geometry::Instrument_const_sptr m_instrument;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADDIFFCAL_H_ */
