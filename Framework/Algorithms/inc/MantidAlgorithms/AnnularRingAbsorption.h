#ifndef MANTID_ALGORITHMS_ANNULARRINGABSORPTION_H_
#define MANTID_ALGORITHMS_ANNULARRINGABSORPTION_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
//-----------------------------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------------------------
namespace Kernel {
class Material;
class V3D;
} // namespace Kernel

namespace Algorithms {

/**
  Constructs a hollow sample shape, defines material for the sample and runs the
  MonteCarloAbsorption algorithm.

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AnnularRingAbsorption : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  void attachSample(API::MatrixWorkspace_sptr &workspace);
  void runCreateSampleShape(API::MatrixWorkspace_sptr &workspace);
  std::string createSampleShapeXML(const Kernel::V3D &upAxis) const;
  const std::string cylinderXML(const std::string &id,
                                const Kernel::V3D &bottomCentre,
                                const double radius, const Kernel::V3D &axis,
                                const double height) const;
  void runSetSampleMaterial(API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr
  runMonteCarloAbsorptionCorrection(const API::MatrixWorkspace_sptr &workspace);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ANNULARRINGABSORPTION_H_ */
