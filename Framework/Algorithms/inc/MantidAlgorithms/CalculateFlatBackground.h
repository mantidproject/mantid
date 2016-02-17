#ifndef MANTID_ALGORITHMS_CALCULATEFLATBACKGROUND_H_
#define MANTID_ALGORITHMS_CALCULATEFLATBACKGROUND_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Finds a constant value fit to an appropriate range of each desired spectrum
    and subtracts that value from the entire spectrum.

    Required Properties:
    <UL>
    <LI> InputWorkspace     - The name of the input workspace. </LI>
    <LI> OutputWorkspace    - The name to give the output workspace. </LI>
    <LI> SpectrumIndexList  - The workspace indices of the spectra to fit
   background to. </LI>
    <LI> StartX             - The start of the flat region to fit to. </LI>
    <LI> EndX               - The end of the flat region to fit to. </LI>
    <LI> Mode               - How to estimate the background number of counts: a
   linear fit or the mean. </LI>
    <LI> OutputMode         - What to return in the Outputworkspace: the
   corrected signal or just the background. </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 5/02/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CalculateFlatBackground : public API::Algorithm {
public:
  /// (Empty) Constructor
  CalculateFlatBackground()
      : API::Algorithm(), m_convertedFromRawCounts(false),
        m_skipMonitors(false), m_nullifyNegative(true), m_progress(nullptr) {}
  /// Virtual destructor
  ~CalculateFlatBackground() override {
    if (m_progress)
      delete m_progress;
    m_progress = nullptr;
  }
  /// Algorithm's name
  const std::string name() const override { return "CalculateFlatBackground"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Finds a constant value fit to an appropriate range of each desired "
           "spectrum and subtracts that value from the entire spectrum.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "SANS;CorrectionFunctions\\BackgroundCorrections";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void convertToDistribution(API::MatrixWorkspace_sptr workspace);
  void restoreDistributionState(API::MatrixWorkspace_sptr workspace);
  void checkRange(double &startX, double &endX);
  void getSpecInds(std::vector<int> &output, const int workspaceTotal);
  double Mean(const API::MatrixWorkspace_const_sptr WS, const int specInd,
              const double startX, const double endX, double &variance) const;
  double LinearFit(API::MatrixWorkspace_sptr WS, int spectrum, double startX,
                   double endX);

  /// variable bin width raw count data must be converted to distributions first
  /// and then converted back, keep track of this
  bool m_convertedFromRawCounts;
  /// the variable which specifies if background should be removed from monitors
  /// too.
  bool m_skipMonitors;
  // if true, negative signals appearing after background removals are set to 0
  bool m_nullifyNegative;
  /// Progress reporting
  API::Progress *m_progress;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CALCULATEFLATBACKGROUND_H_*/
