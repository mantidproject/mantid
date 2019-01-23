// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GETDETECTOROFFSETS_H_
#define MANTID_ALGORITHMS_GETDETECTOROFFSETS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

namespace GetDetectorsOffset {
struct PeakLinearFunction {
  double center;
  double sigma;
  double height;
  double a0;
  double a1;
};
}

/**
 Find the offsets for each detector

 @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
 @date 08/03/2009
 */
class DLLExport GetDetectorOffsets : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GetDetectorOffsets"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates an OffsetsWorkspace containing offsets for each detector. "
           "You can then save these to a .cal file using SaveCalFile.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"GetDetOffsetsMultiPeaks", "CalibrateRectangularDetectors",
            "AlignComponents"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\Calibration";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  /// Call Gaussian as a Child Algorithm to fit the peak in a spectrum
//  double fitSpectra(const int64_t s, bool isAbsolbute);
  double fitSpectra(
      const int64_t s, bool isAbsolbute, const double xmin, const double xmax,
      GetDetectorsOffset::PeakLinearFunction &fit_result,
      const bool use_fit_result);

  double fitPeakSecondTime(
      size_t wi, const bool isAbsolute, const double minimum_peak_height,
      GetDetectorsOffset::PeakLinearFunction &fit_result, bool &mask_it);

  /// Create a function string from the given parameters and the algorithm
  /// inputs
//  API::IFunction_sptr createFunction(const double peakHeight,
//                                     const double peakLoc);
  API::IFunction_sptr createFunction(const double peakHeight,
                                                    const double peakLoc,
                                                    const double peakSigma,
                                                    const double a0,
                                                    const double a1) ;
  /// Read in all the input parameters
  void retrieveProperties();

  ///
  API::ITableWorkspace_sptr GenerateFitResultTable();

  API::MatrixWorkspace_sptr inputW; ///< A pointer to the input workspace
  DataObjects::OffsetsWorkspace_sptr
      outputW;               ///< A pointer to the output workspace
  double m_Xmin = DBL_MAX;   ///< The start of the X range for fitting
  double m_Xmax = -DBL_MIN;  ///< The end of the X range for fitting
  double m_maxOffset = 0.0;  ///< The maximum absolute value of offsets
  double m_dreference = 0.0; ///< The expected peak position in d-spacing (?)
  double m_dideal = 0.0; ///< The known peak centre value from the NIST standard
  /// information
  double m_step = 0.0; ///< The step size
};
} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_GETDETECTOROFFSETS_H_*/
