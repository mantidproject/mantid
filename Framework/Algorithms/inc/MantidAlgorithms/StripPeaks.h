// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** This algorithm calls FindPeaks as a ChildAlgorithm and then subtracts
    all the peaks found from the data, leaving just the 'background'.

    *** IT IS ASSUMED THAT THE FITTING FUNCTION WAS A GAUSSIAN ***

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace.
   </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> fwhm - passed to the FindPeaks ChildAlgorithm</LI>
    <LI> Tolerance - passed to the FindPeaks ChildAlgorithm</LI>
    <LI> WorkspaceIndex - The spectrum from which to remove peaks. Will search
   all spectra if absent.</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/10/2008
*/
class MANTID_ALGORITHMS_DLL StripPeaks : public API::ParallelAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "StripPeaks"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm attempts to find all the peaks in all spectra of a "
           "workspace and subtract them from the data, leaving just the "
           "'background'.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"FindPeaks", "StripVanadiumPeaks"}; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\PeakCorrections;Optimization\\PeakFinding";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  API::ITableWorkspace_sptr findPeaks(const API::MatrixWorkspace_sptr &WS);
  API::MatrixWorkspace_sptr removePeaks(const API::MatrixWorkspace_const_sptr &input,
                                        const API::ITableWorkspace_sptr &peakslist);
  double m_maxChiSq{0.0};
};

} // namespace Algorithms
} // namespace Mantid
