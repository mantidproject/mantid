#ifndef MANTID_ALGORITHMS_INTEGRATEBYCOMPONENT_H_
#define MANTID_ALGORITHMS_INTEGRATEBYCOMPONENT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** IntegrateByComponent : The algorithm integrates up the instrument hierarchy,
  and each pixel will contain the average value for the component

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport IntegrateByComponent : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Averages up the instrument hierarchy.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Integration"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// method to check which spectra should be averaged
  std::vector<std::vector<size_t>> makeMap(API::MatrixWorkspace_sptr countsWS,
                                           int parents);
  /// method to create the map with all spectra
  std::vector<std::vector<size_t>>
  makeInstrumentMap(API::MatrixWorkspace_sptr countsWS);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_INTEGRATEBYCOMPONENT_H_ */
