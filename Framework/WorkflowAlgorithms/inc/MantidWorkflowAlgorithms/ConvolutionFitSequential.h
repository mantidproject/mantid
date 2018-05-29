#ifndef MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIAL_H_
#define MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIAL_H_

#include "MantidAPI/Column.h"
#include "MantidKernel/System.h"
#include "MantidWorkflowAlgorithms/QENSFitSequential.h"

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
class DLLExport ConvolutionFitSequential : public QENSFitSequential {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

protected:
  API::ITableWorkspace_sptr performFit(const std::string &input,
                                       const std::string &output) override;
  std::map<std::string, std::string> getAdditionalLogStrings() const override;
  std::map<std::string, std::string> getAdditionalLogNumbers() const override;

private:
  std::map<std::string, std::string> validateInputs() override;

  bool throwIfElasticQConversionFails() const override;
  bool isFitParameter(const std::string &name) const override;
  void calculateEISF(API::ITableWorkspace_sptr &);

  bool m_deltaUsed;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIAL_H_ */