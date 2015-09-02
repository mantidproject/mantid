#ifndef MANTID_ALGORITHMS_SAMPLEMATERIALCORRECTIONS_H_
#define MANTID_ALGORITHMS_SAMPLEMATERIALCORRECTIONS_H_
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
namespace Mantid {
namespace Algorithms {

/**
  Corrects for the effects of absorption and multiple scattering using different
  algorithms.

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
class MANTID_ALGORITHMS_DLL SampleMaterialCorrections : public API::Algorithm {
public:
  SampleMaterialCorrections();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();

  Kernel::IValidator_sptr createInputWSValidator() const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SAMPLEMATERIALCORRECTIONS_H_ */
