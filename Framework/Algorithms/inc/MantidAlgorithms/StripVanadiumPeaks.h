// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * StripVanadiumPeaks.h
 *
 *  Created on: Sep 10, 2010
 *      Author: janik
 */

#ifndef MANTID_ALGORITHMS_STRIPVANADIUMPEAKS_H_
#define MANTID_ALGORITHMS_STRIPVANADIUMPEAKS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {
/** StripVanadiumPeaks algorithm

    This algorithm takes a list of peak centers (or uses a default one for
   Vanadium peaks)
    and cuts them out by performing a linear fit of the Y values to the left and
   right of the peak:

    - The center of the peak C is specified in d-spacing.
    - A peak width W is specified as a percentage of the d-spacing at the peak
   center.
    - A width of W/2 is averaged on the left and right sides, centered at C +-
   W/2.
    - A linear fit is made from those two value.
    - The Y values between C-W/2 and C+W/2 are filled in with the result of the
   fit.

    @author Janik Zikovsky, SNS
    @date 2010-09-10
*/
class DLLExport StripVanadiumPeaks : public API::Algorithm {
public:
  /// (Empty) Constructor
  StripVanadiumPeaks();
  /// Algorithm's name
  const std::string name() const override { return "StripVanadiumPeaks"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm removes peaks (at vanadium d-spacing positions by "
           "default) out of a background by linearly interpolating over the "
           "expected peak positions.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\PeakCorrections;Optimization\\PeakFinding;"
           "Diffraction\\Corrections";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* STRIPVANADIUMPEAKS_H_ */
