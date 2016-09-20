#ifndef MANTID_ALGORITHMS_IMGGTOMOGRAPHICRECONSTRUCTION_H_
#define MANTID_ALGORITHMS_IMGGTOMOGRAPHICRECONSTRUCTION_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

namespace Mantid {
namespace Algorithms {

/**
  ImggTomographicReconstruction: reconstruct volumes from a sequence
  of 2D projections using tomographic reconstruction methods.

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
class MANTID_ALGORITHMS_DLL ImggTomographicReconstruction
    : public API::Algorithm {
public:
  const std::string name() const override;

  int version() const override;

  const std::string category() const override;

  const std::string summary() const override;

private:
  void init() override;

  void exec() override;

  bool processGroups() override;

  std::map<std::string, std::string> validateInputs() override;

  std::unique_ptr<std::vector<float>>
  prepareProjectionAngles(API::WorkspaceGroup_const_sptr wks, double minAngle,
                          double maxAngle) const;

  std::unique_ptr<std::vector<float>>
  prepareInputData(size_t totalSize, API::WorkspaceGroup_const_sptr wsg);

  std::unique_ptr<std::vector<float>> prepareDataVol(size_t totalSize);

  std::unique_ptr<std::vector<float>> prepareCenters(int cor, size_t totalSize);

  size_t xSizeProjections(API::WorkspaceGroup_const_sptr wks) const;

  size_t pSizeProjections(API::WorkspaceGroup_const_sptr wks) const;

  size_t ySizeProjections(API::WorkspaceGroup_const_sptr wks) const;

  API::WorkspaceGroup_sptr buildOutputWks(const std::vector<float> &dataVol,
                                          size_t xsize, size_t ysize,
                                          size_t sliceSize);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_IMGGTOMOGRAPHICRECONSTRUCTION_H_ */
