#ifndef MANTID_DATAHANDLING_ROTATESOURCE_H_
#define MANTID_DATAHANDLING_ROTATESOURCE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
namespace Mantid {
namespace DataHandling {

/** RotateSource : Moves the source by a given angle taking into account the
  handedness. The centre of rotation is the sample's position and the rotation
  axis (X, Y, Z) is calculated from the instrument geometry as the axis
  perpendicular to the plane defined by the beam and "up" vectors.

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
class DLLExport RotateSource : public API::Algorithm {
public:
  const std::string name() const override { return "RotateSource"; };
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"RotateInstrumentComponent"};
  }
  const std::string category() const override {
    return "DataHandling\\Instrument";
  };
  const std::string summary() const override {
    return "Rotates the source by a given angle";
  };

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_ROTATESOURCE_H_ */