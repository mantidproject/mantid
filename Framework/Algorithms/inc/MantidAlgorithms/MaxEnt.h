#ifndef MANTID_ALGORITHMS_MAXENT_H_
#define MANTID_ALGORITHMS_MAXENT_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
namespace Mantid {
namespace Algorithms {

/** MaxEnt : TODO: DESCRIPTION

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
class DLLExport MaxEnt : public API::Algorithm {
public:
  /// Constructor
  MaxEnt();
  /// Destructor
  virtual ~MaxEnt();

  /// Algorithm's name
  virtual const std::string name() const;
  /// Algorithm's version
  virtual int version() const;
  /// Algorithm's category
  virtual const std::string category() const;
  /// Algorithm's summary
  virtual const std::string summary() const;

private:
  /// Initialise the algorithm's properties
  void init();
  /// Run the algorithm
  void exec();
  /// Validate the input properties
  std::map<std::string, std::string> validateInputs();
  /// Transforms from image space to data space
  std::vector<double> opus(const std::vector<double> &input);
  /// Transforms from data space to image space
  std::vector<double> tropus(const std::vector<double> &input);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENT_H_ */