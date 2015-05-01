#ifndef MANTID_DATAHANDLING_SAVENXSPE_H_
#define MANTID_DATAHANDLING_SAVENXSPE_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/**
 * Saves a workspace into a NeXus/HDF5 NXSPE file.
 *
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

class DLLExport SaveNXSPE : public API::Algorithm {
public:
  /// Constructor
  SaveNXSPE();
  /// Virtual dtor
  virtual ~SaveNXSPE() {}
  virtual const std::string name() const { return "SaveNXSPE"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Writes a MatrixWorkspace to a file in the NXSPE format.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "DataHandling\\Nexus;DataHandling\\SPE;Inelastic";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  // Some constants to be written for masked values.
  /// Value for data if pixel is masked
  static const double MASK_FLAG;
  /// Value for error if pixel is masked
  static const double MASK_ERROR;
  /// file format version
  static const std::string NXSPE_VER;
  /// The size in bytes of a chunk to accumulate to write to the file at once
  static const size_t MAX_CHUNK_SIZE;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_SAVENXSPE_H_
