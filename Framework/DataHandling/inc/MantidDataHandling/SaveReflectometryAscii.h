// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include <fstream>
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {
/**
Saves a file of desired (mft, txt, dat or custom) Ascii format from a 2D
workspace.
*/
class MANTID_DATAHANDLING_DLL SaveReflectometryAscii final : public API::Algorithm {
public:
  /// Algorithm's name. @see Algorithm::name
  const std::string name() const override { return "SaveReflectometryAscii"; }
  /// Algorithm's version. @see Algorithm::version
  int version() const override { return 1; }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "DataHandling\\Text;ILL\\Reflectometry;Reflectometry"; }
  /// Summary of algorithms purpose. @see Algorithm::summary
  const std::string summary() const override { return "Saves a 2D workspace to an ascii file"; }
  /// Algorithm's with similar purpose. @see Algorithm::seeAlso
  const std::vector<std::string> seeAlso() const override { return {"SaveAscii"}; }
  /// Cross-check properties with each other. @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;
  /// Check if input workspace is a group. @see Algorithm::checkGroups
  bool checkGroups() override;

private:
  /// Algorithm initialisation
  void init() override;
  /// Algorithm execution for single MatrixWorkspaces
  void exec() override;
  /// Algorithm execution for WorkspaceGroups
  bool processGroups() override;
  /// Check file validity
  void checkFile(const std::string &filename);
  /// Write the data
  void data();
  /// Print a value to file
  template <typename T> void outputval(const T &val, bool firstColumn = false);
  /// Retrieve sample log value
  std::string sampleLogValue(const std::string &logName);
  /// Retrieve sample log unit
  std::string sampleLogUnit(const std::string &logName);
  /// Write one header line
  void writeInfo(const std::string &logName, const std::string &logNameFixed = "");
  /// Write header
  void header();
  /// Determine the separator
  void separator();
  /// Whether the Q resolution should be included in the output
  bool includeQResolution() const;
  /// Separator
  char m_sep{'\t'};
  /// Filename
  std::string m_filename;
  /// File extension
  std::string m_ext;
  /// Input workspace
  API::MatrixWorkspace_const_sptr m_ws;
  /// Input workspace group
  std::vector<API::MatrixWorkspace_const_sptr> m_group;
  /// Names of the workspaces in a group
  std::vector<std::string> m_wsName;
  /// The output file stream
  std::ofstream m_file;
  /// The angle used to calculate wavelength from momentum exchange, in rad
  double m_theta{0.0};
};
} // namespace DataHandling
} // namespace Mantid
