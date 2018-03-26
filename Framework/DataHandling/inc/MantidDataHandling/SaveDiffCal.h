#ifndef MANTID_DATAHANDLING_SAVEDIFFCAL_H_
#define MANTID_DATAHANDLING_SAVEDIFFCAL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"

namespace H5 {
class Group;
}

namespace Mantid {
namespace DataHandling {

/** SaveDiffCal : TODO: DESCRIPTION

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
class DLLExport SaveDiffCal : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadDiffCal"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  void writeDoubleFieldFromTable(H5::Group &group, const std::string &name);
  void writeIntFieldFromTable(H5::Group &group, const std::string &name);
  void writeIntFieldFromSVWS(H5::Group &group, const std::string &name,
                             DataObjects::SpecialWorkspace2D_const_sptr ws);
  void generateDetidToIndex();
  bool tableHasColumn(const std::string ColumnName) const;

  std::size_t m_numValues{0};
  API::ITableWorkspace_sptr m_calibrationWS;
  std::map<detid_t, size_t> m_detidToIndex;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVEDIFFCAL_H_ */
