// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace DataHandling {
/** An algorithm for finding which detectors are contained within a user defined
   shape within the instrument.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the input Workspace2D on which to perform the
   algorithm </LI>
    <LI> ShapeXML - An XML definition of the shape to be projected within the
   instruemnt of the workspace </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> IncludeMonitors - True/False whether to include monitors in the
   results</LI>
    </UL>

    Output Properties:
    <UL>
    <LI> DetectorList - An array property containing the detectors ids contained
   in the shape </LI>
    </UL>

    @author Nick Draper, Tessella plc
    @date 16/02/2009
*/
class DLLExport FindDetectorsInShape : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FindDetectorsInShape"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Used to find which instrument detectors are contained within a "
           "user-defined 3-D shape.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"MaskDetectorsInShape"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Utility\\Instrument"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
