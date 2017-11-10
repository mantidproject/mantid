#ifndef MANTID_ALGORITHMS_CALCULATEFLATBACKGROUND_H_
#define MANTID_ALGORITHMS_CALCULATEFLATBACKGROUND_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace HistogramData {
class Histogram;
}
namespace Algorithms {
/** Finds a constant background value of each desired spectrum
    and optionally subtracts that value from the entire spectrum.

    Required Properties:
    <UL>
    <LI> InputWorkspace       - The name of the input workspace. </LI>
    <LI> OutputWorkspace      - The name to give the output workspace. </LI>
    <LI> SpectrumIndexList    - The workspace indices of the spectra to fit
   background to. </LI>
    <LI> StartX               - The start of the flat region to fit to. </LI>
    <LI> EndX                 - The end of the flat region to fit to. </LI>
    <LI> AveragingWindowWidth - The width (in bins) of the moving window. </LI>
    <LI> Mode                 - How to estimate the background number of
   counts: a linear fit, the mean, or moving window average. </LI>
    <LI> OutputMode           - What to return in the Outputworkspace: the
   corrected signal or just the background. </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 5/02/2009

    Copyright &copy; 2009-2016 ISIS Rutherford Appleton Laboratory, NScD Oak
    Ridge National Laboratory & European Spallation Source

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
  CalculateFlatBackground() : API::Algorithm() {}
  /// Virtual destructor
  ~CalculateFlatBackground() = default;

  /// Algorithm's name
  const std::string name() const override { return "CalculateFlatBackground"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Finds a constant background value of each desired histogram.";
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
  void Mean(const HistogramData::Histogram &histogram, double &background,
            double &variance, const double startX, const double endX) const;
  void LinearFit(const HistogramData::Histogram &histogram, double &background,
                 double &variance, const double startX, const double endX);
  void MovingAverage(const HistogramData::Histogram &histogram,
                     double &background, double &variance,
                     const size_t windowWidth) const;

  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress = nullptr;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CALCULATEFLATBACKGROUND_H_*/
