#ifndef MANTID_DATAHANDLING_LOADNXSPE_H_
#define MANTID_DATAHANDLING_LOADNXSPE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidGeometry/Objects/CSGObject.h"

namespace Mantid {
namespace DataHandling {

/** LoadNXSPE : Algorithm to load an NXSPE file into a workspace2D. It will
  create a "new" instrument, that can be overwritten later by the LoadInstrument
  algorithm
  Properties:
  <ul>
  <li>Filename  - the name of the file to read from.</li>
  <li>Workspace - the workspace name that will be created and hold the loaded
  data.</li>
  </ul>
  @author Andrei Savici, ORNL
  @date 2011-08-14

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
class DLLExport LoadNXSPE : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadNXSPE"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return " Algorithm to load an NXSPE file into a workspace2D.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SaveNXSPE", "LoadSPE"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return R"(DataHandling\Nexus;DataHandling\SPE;Inelastic\DataHandling)";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  /// Confidence in identifier.
  static int identiferConfidence(const std::string &value);

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Function to return a cuboid shape, with widths dx,dy,dz
  boost::shared_ptr<Geometry::CSGObject> createCuboid(double dx, double dy,
                                                      double dz);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADNXSPE_H_ */
