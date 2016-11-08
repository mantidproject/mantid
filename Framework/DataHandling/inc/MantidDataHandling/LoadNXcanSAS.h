#ifndef MANTID_DATAHANDLING_LOADNXCANSAS_H_
#define MANTID_DATAHANDLING_LOADNXCANSAS_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidAPI/IFileLoader.h"

namespace H5 {
class Group;
}

namespace Mantid {
namespace DataHandling {

/** LoadNXcanSAS : Tries to load an NXcanSAS file type into a Workspace2D.
 *  This can load either 1D or 2D data

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_DATAHANDLING_DLL LoadNXcanSAS
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Constructor
  LoadNXcanSAS();
  /// Virtual dtor
  ~LoadNXcanSAS() override {}
  const std::string name() const override { return "LoadNXcanSAS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an HDF5 NXcanSAS file into a MatrixWorkspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  /// Loads the transmission runs
  void loadTransmission(H5::Group &entry, const std::string &name);
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADNXCANSAS_H_ */
