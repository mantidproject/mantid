// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADMUONLOG_H_
#define MANTID_DATAHANDLING_LOADMUONLOG_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {

namespace DataHandling {
/** @class LoadMuonLog LoadMuonLog.h DataHandling/LoadMuonLog.h

Load ISIS Muon log data from a NeXus file. Sections of NXlog values within the
first run will be loaded.

The algorithm requires an input filename. If this filename is the name of a
NeXus file the algorithm will attempt to read in all the log data (NXlog)
within the first run section of that file.

LoadMuonLog is an algorithm and as such inherits from the Algorithm class,
via NeXus, and overrides the init() & exec() methods.
LoadMuonLog is intended to be used as a child algorithm of
other Loadxxx algorithms, rather than being used directly.

Required Properties:
<UL>
<LI> Filename - The full name of and path of the input ISIS NeXus file </LI>
<LI> Workspace - The workspace to which to append the log data </LI>
</UL>

@author Ronald Fowler, based on LoadLog by Anders Markvardsen, ISIS, RAL
@date 11/08/2008
*/
class DLLExport LoadMuonLog : public API::Algorithm {
public:
  /// Default constructor
  LoadMuonLog();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadMuonLog"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load log data from within Muon Nexus files into a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadLog", "LoadLogPropertyTable"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Logs;Muon\\DataHandling";
  }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// The name and path of an input file. This may be the filename of a raw
  /// datafile or the name of a specific log file.
  std::string m_filename;

  /// convert string to lower case
  std::string stringToLower(std::string strToConvert);

  /// check if first 19 characters of a string is data-time string according to
  /// yyyy-mm-ddThh:mm:ss
  bool isDateTimeString(const std::string &str);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADMUONLOG_H_*/
