#ifndef MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIAL_H_
#define MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIAL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** ConvolutionFitSequential : Performs a sequential fit for a convolution
  workspace

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
class DLLExport ConvolutionFitSequential : public API::DataProcessorAlgorithm {
public:
  ConvolutionFitSequential();
  ~ConvolutionFitSequential() override;

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  bool checkForTwoLorentz(const std::string & /*subFunction*/);
  std::vector<std::string> findValuesFromFunction(const std::string & /*function*/);
  std::vector<std::string> searchForFitParams(const std::string & /*suffix*/,
                                              const std::vector<std::string> & /*columns*/);
  std::vector<double> squareVector(std::vector<double> /*target*/);
  std::vector<double> cloneVector(const std::vector<double> & /*original*/);
  void convertInputToElasticQ(API::MatrixWorkspace_sptr & /*inputWs*/, const std::string & /*wsName*/);
  void calculateEISF(API::ITableWorkspace_sptr & /*tableWs*/);
  std::string convertBackToShort(const std::string & /*original*/);
  std::string convertFuncToShort(const std::string & /*original*/);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIAL_H_ */