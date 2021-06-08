// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Run.h"

namespace Mantid {

namespace DataHandling {
/** @class RemoveLogs RemoveLogs.h DataHandling/RemoveLogs.h

    Load ISIS log file(s). Assumes that a log file originates from a
    PC (not VMS) environment, i.e. the log files to be loaded are assumed
    to have the extension .txt. Its filename is assumed to starts with the raw
   data
    file identifier followed by the character '_', and a log file is assumed to
   have a
    format of two columns, where the first column consists of data-time strings
   of the
    ISO 8601 form and the second column consists of either numbers or strings
   that may
    contain spaces.

    The algoritm requires an input filename. If this filename is the name of a
    raw datafile the algorithm will attempt to read in all the log files
   associated
    with that log file. Otherwise it will assume the filename specified is the
    filename of a specific log file.

    RemoveLogs is an algorithm and as such inherits from the Algorithm class,
    via DataHandlingCommand, and overrides the init() & exec() methods.
    RemoveLogs is intended to be used as a child algorithm of
    other Loadxxx algorithms, rather than being used directly.

    Required Properties:
    <UL>
    <LI> Filename - The filename (including its full or relative path) of either
   an ISIS log file
    or an ISIS raw file. If a raw file is specified all log files associated
   with that raw file
    are loaded into the specified workspace. The file extension must either be
   .raw or .s when
    specifying a raw file, and at least 10 characters long. </LI>
    <LI> Workspace - The workspace to which the log data is appended </LI>
    </UL>

    @author Vickie Lynch, SNS
    @date 26/04/2012
*/
class DLLExport RemoveLogs : public API::Algorithm {
public:
  /// Default constructor
  RemoveLogs();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RemoveLogs"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Remove logs from a workspace."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"RenameLog", "DeleteLog"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Logs"; }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
