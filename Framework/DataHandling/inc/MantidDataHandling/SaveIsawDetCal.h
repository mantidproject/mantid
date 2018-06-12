#ifndef MANTID_DATAHANDLING_SAVEISAWDETCAL_H_
#define MANTID_DATAHANDLING_SAVEISAWDETCAL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/** Saves an instrument with RectangularDetectors to an ISAW .DetCal file.

  @author Janik Zikovsky
  @date 2011-10-03

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
class DLLExport SaveIsawDetCal : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveIsawDetCal"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves an instrument with RectangularDetectors to an ISAW .DetCal "
           "file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadIsawDetCal"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Diffraction\\DataHandling;DataHandling\\Isaw";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// find position for rectangular and non-rectangular
  Kernel::V3D findPixelPos(std::string bankName, int col, int row);
  void sizeBanks(std::string bankName, int &NCOLS, int &NROWS, double &xsize,
                 double &ysize);
  Geometry::Instrument_const_sptr inst;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVEISAWDETCAL_H_ */
