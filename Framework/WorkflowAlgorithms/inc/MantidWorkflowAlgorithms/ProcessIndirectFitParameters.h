#ifndef MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_
#define MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ProcessIndirectFitParameters : Convert a parameter table output by
  PlotPeakByLogValue to a MatrixWorkspace. This will make a spectrum for each
  parameter name using the x_column vairable as the x values for the spectrum.

  @author Elliot Oram, ISIS, RAL
  @date 12/08/2015

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
class DLLExport ProcessIndirectFitParameters : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::size_t getStartRow() const;
  std::size_t getEndRow(std::size_t maximum) const;
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_ */