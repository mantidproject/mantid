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
  const std::string name() const override {
    return "IdentifyNoisyDetectors";
  } ///< @return the algorithms name
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm creates a single-column workspace where the Y "
           "values are populated withs 1s and 0s, 0 signifying that the "
           "detector is to be considered 'bad' based on the method described "
           "below.";
  }

  const std::string category() const override {
    return "Diagnostics";
  } ///< @return the algorithms category
  int version() const override {
    return (1);
  } ///< @return version number of algorithm

  const std::vector<std::string> seeAlso() const override {
    return {"CreatePSDBleedMask"};
  }

private:
  void init() override; ///< Initialise the algorithm. Declare properties, etc.
  void exec() override; ///< Executes the algorithm.

  void getStdDev(API::Progress &progress,
                 Mantid::API::MatrixWorkspace_sptr valid,
                 Mantid::API::MatrixWorkspace_sptr values);
};
}
}
#endif
