#ifndef MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_
#define MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
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
  ProcessIndirectFitParameters();
  virtual ~ProcessIndirectFitParameters();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();
  std::vector<std::string> listToVector(std::string &);
  std::vector<std::string> searchForFitParams(const std::string &,
                                              const std::vector<std::string> &);
  std::vector<std::vector<std::string>>
  reorder2DVector(const std::vector<std::vector<std::string>> &);
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERS_H_ */