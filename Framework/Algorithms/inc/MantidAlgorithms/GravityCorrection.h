#ifndef GRAVITYCORRECTION_H_
#define GRAVITYCORRECTION_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/*! GravityCorrection : Correction of time-of-flight values and final angles, i.e. angles between the reflected beam and the sample, due to gravity for
 2D Workspaces.
 Copyright &copy; 2014-2017 ISIS Rutherford Appleton Laboratory, NScD Oak
 Ridge National Laboratory & European Spallation Source
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

class DLLExport GravityCorrection : public Mantid::API::Algorithm {
public:
  /// Empty constructor
  GravityCorrection() = default;
  /// Algorithm's name. @see Algorithm::name
  const std::string name() const override { return "GravityCorrection"; }
  /// Algorithm's version. @see Algorithm::version
  int version() const override { return (1); }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry"; }
  /// Algorithm's summary. @see Algorith::summary
  const std::string summary() const override {
    return "Correction of time-of-flight values and final angles, i.e. angles between the reflected beam and the sample, due to gravity for "
           "2DWorkspaces.";
  }
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*GRAVITYCORRECTION_H_*/
