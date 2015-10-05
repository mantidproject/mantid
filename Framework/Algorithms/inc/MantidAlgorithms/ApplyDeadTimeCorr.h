#ifndef MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_
#define MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** ApplyDeadTimeCorr : The dead-time is here applied according to the
  non-extendable paralyzable dead time model.

  @author Robert Whitley, RAL
  @date 2011-11-30

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
class DLLExport ApplyDeadTimeCorr : public API::Algorithm {
public:
  /// Default constructor
  ApplyDeadTimeCorr() : API::Algorithm(){};
  /// Destructor
  virtual ~ApplyDeadTimeCorr(){};
  /// Algorithm's name for identification
  virtual const std::string name() const { return "ApplyDeadTimeCorr"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Apply deadtime correction to each spectra of a workspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Muon;CorrectionFunctions\\EfficiencyCorrections";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_ */
