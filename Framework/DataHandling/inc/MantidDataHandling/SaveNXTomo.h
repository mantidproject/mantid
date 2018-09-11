#ifndef MANTID_DATAHANDLING_SAVENXTOMO_H_
#define MANTID_DATAHANDLING_SAVENXTOMO_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <vector>

namespace Mantid {
namespace DataHandling {

/**
 * Saves a workspace into a NeXus/HDF5 NXTomo file.
 * File format is defined here:
 *http://download.nexusformat.org/sphinx/classes/applications/NXtomo.html
 *
 * Required properties:
 * <ul>
 * <li> InputWorkspace - The workspace to save. </li>
 * <li> Filename - The filename for output </li>
 * </ul>
 *
 * @author John R Hill, RAL
 * @date 10/09/2014
 *
 * This file is part of Mantid.
 *
 *   Mantid is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Mantid is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   File change history is stored at: <https://github.com/mantidproject/mantid>
 *   Code Documentation is available at: <http://doxygen.mantidproject.org>
 *
 */

class DLLExport SaveNXTomo : public API::Algorithm {
public:
  SaveNXTomo();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveNXTomo"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves one or more workspaces (of type MatrixWorkspace) to a file "
           "in the NXTomo format.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveNexusProcessed"};
  }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Nexus;DataHandling\\Imaging;"
           "Diffraction\\DataHandling";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code : Single workspace
  void exec() override;
  /// Alternative execution code when operating on a WorkspaceGroup
  bool processGroups() override;

  /// Creates the format for the output file if it doesn't exist
  ::NeXus::File setupFile();

  /// Writes a single workspace into the file
  void writeSingleWorkspace(const DataObjects::Workspace2D_sptr workspace,
                            ::NeXus::File &nxFile);

  /// Write various pieces of data from the workspace log with checks on the
  /// structure of the nexus file
  void writeLogValues(const DataObjects::Workspace2D_sptr workspace,
                      ::NeXus::File &nxFile, int thisFileInd);
  void writeIntensityValue(const DataObjects::Workspace2D_sptr workspace,
                           ::NeXus::File &nxFile, int thisFileInd);
  void writeImageKeyValue(const DataObjects::Workspace2D_sptr workspace,
                          ::NeXus::File &nxFile, int thisFileInd);

  /// Main exec routine, called for group or individual workspace processing.
  void processAll();

  // Include error data in the written file
  bool m_includeError;
  bool m_overwriteFile;
  size_t m_spectraCount;
  std::vector<int64_t> m_slabStart;
  std::vector<int64_t> m_slabSize;
  /// The filename of the output file
  std::string m_filename;
  // Dimensions for axis in nxTomo file.
  std::vector<int64_t> m_dimensions;
  // Infinite file range dimensions / for use with makeData data and error
  std::vector<int64_t> m_infDimensions;

  /// file format version
  static const std::string NXTOMO_VER;

  std::vector<DataObjects::Workspace2D_sptr> m_workspaces;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_SAVENXTOMO_H_
