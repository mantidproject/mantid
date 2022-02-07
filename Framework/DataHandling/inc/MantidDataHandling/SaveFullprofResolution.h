// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** SaveFullprofResolution : TODO: DESCRIPTION
 */
class DLLExport SaveFullprofResolution : public API::Algorithm {
public:
  SaveFullprofResolution();
  /// Algorithm's name
  const std::string name() const override { return "SaveFullprofResolution"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a Table workspace, which contains peak profile parameters' "
           "values, to a Fullprof resolution (.irf) file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveFocusedXYE"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling;DataHandling\\Text"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Write the header information

  /// Process properties
  void processProperties();

  /// Write out string for profile 10 .irf file
  std::string toProf10IrfString();

  /// Write out string for profile 9 .irf file
  std::string toProf9IrfString();

  /// Parse input workspace to map of parameters
  void parseTableWorkspace();

  /// Check wether a profile parameter map has the parameter
  bool has_key(std::map<std::string, double> profmap, const std::string &key);

  /// Map containing the name of value of each parameter required by .irf file
  std::map<std::string, double> m_profileParamMap;

  /// Input table workspace
  DataObjects::TableWorkspace_sptr m_profileTableWS;

  /// Output Irf file name
  std::string m_outIrfFilename;

  /// Bank to write
  int m_bankID;

  /// Profile number
  int m_fpProfileNumber;

  /// Append to existing file
  bool m_append;
};

} // namespace DataHandling
} // namespace Mantid
