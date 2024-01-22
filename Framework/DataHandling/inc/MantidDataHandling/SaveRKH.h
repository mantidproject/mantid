// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include <fstream>

namespace Mantid {
namespace DataHandling {
/** Saves a workspace in the RKH file format

    Required properties:
    <UL>
    <LI> InputWorkspace - The name workspace to save.</LI>
    <LI> Filename - The path save the file</LI>
    <LI> Append - Whether to append to a file that already exists (true, the
   default), or overwrite</LI>
    </UL>

    @author Martyn Gigg, Tessella Support Services plc
    @date 26/01/2009
 */
class MANTID_DATAHANDLING_DLL SaveRKH final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SaveRKH"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a MatrixWorkspace to a file in the ISIS RKH format (for 1D or 2D data).";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"LoadRKH"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Text"; }

  /// Constants used in RKH files
  enum FileConstants {
    Q_CODE = 6, ///< this is the integer code the RKH file format associates
    /// with the unit Q
    LINE_LENGTH = 8 ///< the maximum number of numbers that a line can contain
  };

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void writeHeader();
  void write1D();
  void write2D();

  /// The input workspace
  API::MatrixWorkspace_const_sptr m_workspace;
  /// Whether this is a 2D dataset
  bool m_2d{false};
  /// The output filehandle
  std::ofstream m_outRKH;
};
} // namespace DataHandling
} // namespace Mantid
