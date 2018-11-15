// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATELOGPROPERTYTABLE_H_
#define MANTID_ALGORITHMS_CREATELOGPROPERTYTABLE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/Algorithm.h"

#include <string>

namespace Mantid {
namespace Algorithms {

class DLLExport CreateLogPropertyTable : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CreateLogPropertyTable"; };
  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"CopyLogs"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

  /// Algorithm's summary
  const std::string summary() const override {
    return "  Takes a list of workspaces and a list of log property names.  "
           "For each workspace, the Run info is inspected and "
           "all log property values are used to populate a resulting output "
           "TableWorkspace.";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATELOGPROPERTYTABLE_H_ */
