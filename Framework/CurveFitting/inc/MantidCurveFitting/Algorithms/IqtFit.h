#ifndef MANTID_ALGORITHMS_IQTFITSEQUENTIAL_H_
#define MANTID_ALGORITHMS_IQTFITSEQUENTIAL_H_

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/**
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
template <typename Base> class DLLExport IqtFit : public Base {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

protected:
  std::vector<API::MatrixWorkspace_sptr> getWorkspaces() const override;

private:
  double getStartX(std::size_t index) const;
  double getEndX(std::size_t index) const;

  std::map<std::string, std::string> validateInputs() override;
  bool isFitParameter(const std::string &name) const override;
  bool throwIfElasticQConversionFails() const override;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif
