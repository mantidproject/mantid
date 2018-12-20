// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_GROUPDETECTORS_H_
#define MANTID_DATAHANDLING_GROUPDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/** An algorithm for grouping detectors and the spectra associated with them
    into a single DetectorGroup and spectrum.
    This algorithm can only be used on a workspace that has common X bins.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the (input & output) Workspace2D on which to
   perform the algorithm </LI>
    </UL>

    Optional Properties (Only one of these should be set. Priority to highest
   listed below if more than one is set.):
    <UL>
    <LI> SpectraList - An ArrayProperty containing a list of spectra to combine
   </LI>
    <LI> DetectorList - An ArrayProperty containing a list of detector IDs to
   combine </LI>
    <LI> WorkspaceIndexList - An ArrayProperty containing the workspace indices
   to combine </LI>
    </UL>

    Output Properties:
    <UL>
    <LI> ResultIndex - The workspace index containing the grouped spectra </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 17/04/2008
*/
class DLLExport GroupDetectors : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GroupDetectors"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Sums spectra bin-by-bin, equivalent to grouping the data from a "
           "set of detectors.  Individual groups can be specified by passing "
           "the algorithm a list of spectrum numbers, detector IDs or "
           "workspace indices. Many spectra groups can be created in one "
           "execution via an input file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Grouping"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_GROUPDETECTORS_H_*/
