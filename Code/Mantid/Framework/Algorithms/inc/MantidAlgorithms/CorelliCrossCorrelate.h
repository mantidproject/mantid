#ifndef MANTID_ALGORITHMS_CORELLICROSSCORRELATE_H_
#define MANTID_ALGORITHMS_CORELLICROSSCORRELATE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** CorelliCrossCorrelate : TODO: DESCRIPTION

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
  National Laboratory

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
class DLLExport CorelliCrossCorrelate : public API::Algorithm {
public:
  CorelliCrossCorrelate();
  virtual ~CorelliCrossCorrelate();

  virtual const std::string name() const { return "CorelliCrossCorrelate"; };
  virtual int version() const { return 1; };
  virtual const std::string category() const { return "Diffraction;Events"; };
  virtual const std::string summary() const {
    return "Cross-correlation calculation for the elastic signal from Corelli.";
  };

private:
  std::map<std::string, std::string> validateInputs();
  void init();
  void exec();

  /// Input workspace
  DataObjects::EventWorkspace_const_sptr inputWS;
  DataObjects::EventWorkspace_sptr outputWS;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CORELLICROSSCORRELATE_H_ */
