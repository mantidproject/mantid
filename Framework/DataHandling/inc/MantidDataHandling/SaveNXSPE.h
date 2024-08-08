// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Repository : https://github.com/mantidproject/mantid
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

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

class MANTID_DATAHANDLING_DLL SaveNXSPE final : public API::Algorithm {
public:
  /// Constructor
  SaveNXSPE();
  const std::string name() const override { return "SaveNXSPE"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Writes a MatrixWorkspace to a file in the NXSPE format."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"LoadNXSPE", "SaveSPE"}; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return R"(DataHandling\Nexus;DataHandling\SPE;Inelastic\DataHandling)";
  }
  // public for testing only
  std::vector<double> getIndirectEfixed(const Mantid::API::MatrixWorkspace_sptr &inputWS) const;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

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
