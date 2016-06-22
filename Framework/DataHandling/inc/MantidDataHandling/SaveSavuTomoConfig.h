#ifndef MANTID_DATAHANDLING_SAVESAVUTOMOCONFIG_H_
#define MANTID_DATAHANDLING_SAVESAVUTOMOCONFIG_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace Mantid {
namespace DataHandling {

/**
 * Saves a configuration for a tomographic reconstruction into a
 * NeXus/HDF5 file.
 *
 * Operates on table workspaces, with each row representing a plugin
 * definition to add.
 *
 * Columns:4: id/params/name/cite
 *
 * Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory,
 * NScD Oak Ridge National Laboratory & European Spallation Source
 *
 * This file is part of Mantid.
 *
 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class DLLExport SaveSavuTomoConfig : public API::Algorithm {
public:
  SaveSavuTomoConfig();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveSavuTomoConfig"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes a configuration file for a tomographic reconstruction job.";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Imaging;";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code : Single workspace
  void exec() override;

  /// basic check on a table workspace properties
  bool tableLooksGenuine(const API::ITableWorkspace_sptr &tws);

  /// get table workspaces (checking the workspace names given)
  std::vector<API::ITableWorkspace_sptr>
  checkTables(const std::vector<std::string> &workspaces);
  /// write savu tomo config file
  void saveFile(const std::string fname,
                const std::vector<API::ITableWorkspace_sptr> &wss);

  // Number of info entries to read from the input table workspaces
  unsigned int m_pluginInfoCount;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_SAVESAVUTOMOCONFIG_H_
