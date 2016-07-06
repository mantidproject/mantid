#ifndef MANTID_ALGORITHMS_CREATEUSERDEFINEDBACKGROUND_H_
#define MANTID_ALGORITHMS_CREATEUSERDEFINEDBACKGROUND_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
namespace Mantid {
namespace Kernel {
class Interpolation;
}
namespace Algorithms {

/** CreateUserDefinedBackground : Given an input workspace containing data with
  a background and a table of user-selected points defining the background,
  creates a new workspace containing background data that can be subtracted
  from the original data.

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
class MANTID_ALGORITHMS_DLL CreateUserDefinedBackground final
    : public API::Algorithm {
public:
  /// Return name of algorithm
  const std::string name() const override final;
  /// Version number
  int version() const override final;
  /// Category algorithm belongs to
  const std::string category() const override final;
  /// Description of algorithm
  const std::string summary() const override final;

private:
  /// Declare properties and initialise algorithm
  void init() override final;
  /// Execute algorithm
  void exec() override final;
  /// Verify input properties and return errors
  std::map<std::string, std::string> validateInputs() override final;
  /// Sort table and remove blank rows
  void cleanUpTable(API::ITableWorkspace_sptr &table) const;
  /// Extend background to cover range of data
  void
  extendBackgroundToData(API::ITableWorkspace_sptr &background,
                         const API::MatrixWorkspace_const_sptr &data) const;
  /// Create new background workspace with data interpolated from table
  API::MatrixWorkspace_sptr
  createBackgroundWorkspace(const API::ITableWorkspace_const_sptr &background,
                            const API::MatrixWorkspace_const_sptr &data) const;
  /// Set up and return an interpolator object ready for use
  Kernel::Interpolation
  getInterpolator(const API::ITableWorkspace_const_sptr &background,
                  const API::MatrixWorkspace_const_sptr &workspace) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATEUSERDEFINEDBACKGROUND_H_ */