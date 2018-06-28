#ifndef MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIES_H_
#define MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIES_H_

#include "MantidDataHandling/CreatePolarizationEfficienciesBase.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

/** JoinISISPolarizationEfficiencies : Joins reflectometry polarization
  efficiency correction factors to form a single matrix workspace.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_DATAHANDLING_DLL JoinISISPolarizationEfficiencies
    : public CreatePolarizationEfficienciesBase {
public:
  const std::string name() const override;
  int version() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &props) override;
  API::MatrixWorkspace_sptr
  createEfficiencies(std::vector<std::string> const &labels,
                     std::vector<API::MatrixWorkspace_sptr> const &workspaces);
  std::vector<API::MatrixWorkspace_sptr> interpolateWorkspaces(
      std::vector<API::MatrixWorkspace_sptr> const &workspaces);
  API::MatrixWorkspace_sptr
  interpolatePointDataWorkspace(API::MatrixWorkspace_sptr ws,
                                size_t const maxSize);
  API::MatrixWorkspace_sptr
  interpolateHistogramWorkspace(API::MatrixWorkspace_sptr ws,
                                size_t const maxSize);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIES_H_ */
