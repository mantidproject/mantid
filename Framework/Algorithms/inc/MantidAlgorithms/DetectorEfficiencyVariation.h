// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorDiagnostic.h"

namespace Mantid {
namespace Algorithms {
/**
   Required Properties:
   <UL>
   <LI> WhiteBeamBase - Name of a white beam vanadium workspace </LI>
   <LI> WhiteBeamCompare - Name of a matching second white beam vanadium run
   from the same instrument </LI>
   <LI> OutputWorkspace - A MaskWorkpace where each spectra that failed the test
   is masked </LI>
   <LI> Variation - Identify spectra whose total number of counts has changed by
   more than this factor of the median change between the two input workspaces
   </LI>
   </UL>

   Optional Properties:
   <UL>
   <LI> StartWorkspaceIndex - The index number of the first entry in the
   Workspace to include in the calculation </LI>
   <LI> EndWorkspaceIndex - The index number of the last entry in the Workspace
   to include in the calculation </LI>
   <LI> RangeLower - No bin with a boundary at an x value less than this will be
   included in the summation used to decide if a detector is 'bad' </LI>
   <LI> RangeUpper - No bin with a boundary at an x value higher than this value
   will be included in the summation used to decide if a detector is 'bad' </LI>
   </UL>

   @author Steve D Williams, ISIS Facility Rutherford Appleton Laboratory
   @date 15/06/2009
*/
class MANTID_ALGORITHMS_DLL DetectorEfficiencyVariation : public DetectorDiagnostic {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "DetectorEfficiencyVariation"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Compares two white beam vanadium workspaces from the same "
           "instrument to find detectors whose efficiencies have changed "
           "beyond a threshold.";
  }

  const std::string category() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"DetectorDiagnostic"}; }

protected:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Loads and checks the values passed to the algorithm
  void retrieveProperties(API::MatrixWorkspace_sptr &whiteBeam1, API::MatrixWorkspace_sptr &whiteBeam2,
                          double &variation, int &minSpec, int &maxSpec);
  /// Apply the detector test criterion
  int doDetectorTests(const API::MatrixWorkspace_const_sptr &counts1, const API::MatrixWorkspace_const_sptr &counts2,
                      const double average, double variation);

private:
};

} // namespace Algorithms
} // namespace Mantid
