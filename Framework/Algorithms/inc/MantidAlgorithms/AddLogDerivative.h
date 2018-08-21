#ifndef MANTID_ALGORITHMS_ADDLOGDERIVATIVE_H_
#define MANTID_ALGORITHMS_ADDLOGDERIVATIVE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

/** Takes an existing sample log, and calculates its first or second
 * derivative, and adds it as a new log.

  @author Janik Zikovsky
  @date 2011-09-16

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AddLogDerivative : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "AddLogDerivative"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Add a sample log that is the first or second derivative of an "
           "existing sample log.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"AddSampleLog"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Logs"; }

  static Mantid::Kernel::TimeSeriesProperty<double> *
  makeDerivative(API::Progress &progress,
                 Mantid::Kernel::TimeSeriesProperty<double> *input,
                 const std::string &name, int numDerivatives);

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ADDLOGDERIVATIVE_H_ */
