#ifndef MANTID_ALGORITHMS_IDENTIFYNOISYDETECTORS
#define MANTID_ALGORITHMS_IDENTIFYNOISYDETECTORS

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
  Identifies "bad" detectors based on their standard deviation, and how this
  differs from the
  standard deviation of other detectors. Runs through the process three times to
  get a narrower
  view.
  @author Michael Whitty
  @date 24/01/2011

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport IdentifyNoisyDetectors : public API::Algorithm {
public:
  IdentifyNoisyDetectors() : API::Algorithm() {} ///< Empty constructor
  virtual ~IdentifyNoisyDetectors() {}           ///< Empty destructor

  virtual const std::string name() const {
    return "IdentifyNoisyDetectors";
  } ///< @return the algorithms name
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "This algorithm creates a single-column workspace where the Y "
           "values are populated withs 1s and 0s, 0 signifying that the "
           "detector is to be considered 'bad' based on the method described "
           "below.";
  }

  virtual const std::string category() const {
    return "Diagnostics";
  } ///< @return the algorithms category
  virtual int version() const {
    return (1);
  } ///< @return version number of algorithm

private:
  void init(); ///< Initialise the algorithm. Declare properties, etc.
  void exec(); ///< Executes the algorithm.

  void getStdDev(Mantid::API::MatrixWorkspace_sptr valid,
                 Mantid::API::MatrixWorkspace_sptr values);
};
}
}
#endif
