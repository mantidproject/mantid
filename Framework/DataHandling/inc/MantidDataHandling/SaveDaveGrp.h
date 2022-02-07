// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** SaveDaveGrp : @class SaveAscii SaveAscii.h DataHandling/SaveAscii.h

  Saves a workspace to a DAVE grp file. See
  http://www.ncnr.nist.gov/dave/documentation/ascii_help.pdf
  Properties:
  <ul>
        <li>Filename - the name of the file to write to.  </li>
        <li>Workspace - the workspace name to be saved.</li>
        <li>ToMicroeV - transform any energy axis into micro eV (optional) </li>
  </ul>

  @author Andrei Savici, ORNL
  @date 2011-07-22
*/
class DLLExport SaveDaveGrp : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveDaveGrp"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to DAVE grouped data format file.See "
           "http://www.ncnr.nist.gov/dave/documentation/ascii_help.pdf";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"LoadDaveGrp"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Text;Inelastic\\DataHandling"; }
  /// Algorithm's aliases
  const std::string alias() const override { return "SaveDASC"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
