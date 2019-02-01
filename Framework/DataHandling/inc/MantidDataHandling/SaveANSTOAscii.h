// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVEANSTOASCII_H_
#define MANTID_DATAHANDLING_SAVEANSTOASCII_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidDataHandling/AsciiPointBase.h"

namespace Mantid {
namespace DataHandling {
/**
Saves a file in Ansto format and from a 2D workspace
(Workspace2D class). SaveANSTOAscii is an algorithm but inherits from the
AsciiPointBase class which provides the main implementation for the init() &
exec() methods.
Output is tab delimited Ascii point data with dq/q.
*/
class DLLExport SaveANSTOAscii : public DataHandling::AsciiPointBase,
                                 public API::DeprecatedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveANSTOAscii"; }
  /// Lines should not start with a separator
  bool leadingSep() override { return false; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to a ascii file.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"SaveAscii"};
  }
  /// Constructor
  SaveANSTOAscii() {
    this->useAlgorithm("SaveReflectometryAscii");
    this->deprecatedDate("2018-06-29");
  }

private:
  /// Return the file extension this algorthm should output.
  std::string ext() override { return ".txt"; }
  /// only separator property required, nothing else
  void extraProps() override { appendSeparatorProperty(); }
  /// no extra information required so override blank
  void extraHeaders(std::ofstream &file) override { UNUSED_ARG(file); };
};

} // namespace DataHandling
} // namespace Mantid
#endif /*  MANTID_DATAHANDLING_SAVEANSTO_H_  */
