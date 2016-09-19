#ifndef MANTID_DATAHANDLING_APPLYCALIBRATION_H_
#define MANTID_DATAHANDLING_APPLYCALIBRATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Geometry {
class Instrument;
}

namespace Algorithms {
/**

Update detector positions from input table workspace. The positions are updated
as absolute positions and so this update can be repeated.


@author Karl Palmen

Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
class DLLExport ApplyCalibration : public API::Algorithm {
public:
  /// Default constructor
  ApplyCalibration();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ApplyCalibration"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Update detector positions from input table workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  } // Needs to change

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  /// Set the detector position from the calibration table
  void setDetectorPosition(const Geometry::Instrument_const_sptr &instrument,
                           const int detID, Mantid::Kernel::V3D pos,
                           bool sameParent);

  /// A pointer to the parameter map being modified
  Geometry::ParameterMap *m_pmap;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_APPLYCALIBRATION_H_*/
