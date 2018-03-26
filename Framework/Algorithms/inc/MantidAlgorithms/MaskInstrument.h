#ifndef MANTID_ALGORITHMS_MASKINSTRUMENT_H_
#define MANTID_ALGORITHMS_MASKINSTRUMENT_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace Algorithms {

/** Mask specified detectors in an instrument. This is does NOT clear the data
  in associated spectra in the workspace.  To clear the data manually
  `ClearMaskedSpectra` can be called.

  @author Simon Heybrock
  @date 2017

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
class MANTID_ALGORITHMS_DLL MaskInstrument : public API::DistributedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MaskDetectors", "ClearMaskedSpectra"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MASKINSTRUMENT_H_ */
