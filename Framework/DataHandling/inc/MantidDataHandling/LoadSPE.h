#ifndef MANTID_DATAHANDLING_LOADSPE_H_
#define MANTID_DATAHANDLING_LOADSPE_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"

namespace Mantid {
namespace DataHandling {
/**
  Loads an SPE format file into a Mantid workspace.

  Required properties:
  <UL>
  <LI> Filename - The SPE format file to be read </LI>
  <LI> Workspace - The name to give to the output workspace </LI>
  </UL>

  @author Russell Taylor, Tessella plc
  @date 02/02/2010

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
class DLLExport LoadSPE : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadSPE"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a file written in the spe format.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveSPE"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\SPE;Inelastic\\DataHandling";
  }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;

  void readHistogram(FILE *speFile, API::MatrixWorkspace_sptr workspace,
                     size_t index);
  void reportFormatError(const std::string &what);

  std::string m_filename; ///< The file to load
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadSPE_H_*/
