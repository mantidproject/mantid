// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
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
Takes a workspace as input and finds all the detectors with solid angle
corrected signals
that deviate far enough from median value of all detectors to be suspious.  The
factors used
to define detectors as reading too low or reading too high are selectable by
setting the
"Low" and "High" properties.  By default the median value is calculated using
the entire
spectrum but a region can be selected by setting startX and endX.

The output workspace is a MaskWorkspace which contains 1 bin per spectra where 0
denotes a
masked spectra and 1 denotes and unmasked spectra

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
</UL>

Optional Properties:
<UL>
<LI> LowThreshold - Detectors with signals of this proportion of the median
value, or less, will be labeled as reading low (default 0.1)</LI>
<LI> HighThreshold - Detectors with signals this number of times, or more, than
the median signal will be labeled as reading high (default 1.5)</LI>
<LI> StartSpectrum - The index number of the first spectrum to include in the
calculation (default 0)</LI>
<LI> EndSpectrum - The index number of the last spectrum to include in the
calculation (default the last histogram) </LI>
<LI> RangeLower - Start the integration at the bin above the one that this value
is in (default: the start of each spectrum)</LI>
<LI> RangeUpper - Stop the integration at the bin before the one that contains
this x value (default: the end of each spectrum)</LI>
</UL>
*/
class MANTID_ALGORITHMS_DLL MedianDetectorTest : public DetectorDiagnostic {
public:
  /// Default constructor initialises all values to zero and runs the base class
  /// constructor
  MedianDetectorTest();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MedianDetectorTest"; }
  const std::string category() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"DetectorDiagnostic"}; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  // The different steps of the calculation, all called by exec()
  /// Loads and checks the values passed to the algorithm
  void retrieveProperties();
  /// Calculates the sum of solid angles of detectors for each histogram
  API::MatrixWorkspace_sptr getSolidAngles(int firstSpec, int lastSpec);
  /// Mask the outlier values to get a better median value
  int maskOutliers(const std::vector<double> &medianvec, const API::MatrixWorkspace_sptr &countsWS,
                   std::vector<std::vector<size_t>> indexmap);
  /// Do the tests and mask those that fail
  int doDetectorTests(const API::MatrixWorkspace_sptr &countsWS, const std::vector<double> &medianvec,
                      const std::vector<std::vector<size_t>> &indexmap, const API::MatrixWorkspace_sptr &maskWS);

  API::MatrixWorkspace_sptr m_inputWS;
  /// The proportion of the median value below which a detector is considered
  /// under-reading
  double m_loFrac;
  /// The factor of the median value above which a detector is considered
  /// over-reading
  double m_hiFrac;
  /// The index of the first spectrum to calculate
  int m_minWsIndex;
  /// The index of the last spectrum to calculate
  int m_maxWsIndex;
  /// Start point for integration
  double m_rangeLower;
  /// End point for integration
  double m_rangeUpper;
  /// flag for solid angle correction
  bool m_solidAngle;
};

} // namespace Algorithms
} // namespace Mantid
