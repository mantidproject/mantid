// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {
/**Takes a workspace as input and applies a
apodization function and/or padding

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Spectra - The spectra to be adjusted (by default all spectra are done)</LI>
<LI> ApodizationFunction - the apodization function to use </LI>
<LI> decayConstant - the decay constant for the apodization function</LI>
<LI> padding - the number of times to extend the data range by zeros </LI>
</UL>


@author Anthony Lim
@date 10/08/2017
*/
class MANTID_ALGORITHMS_DLL PaddingAndApodization : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PaddingAndApodization"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm applies apodization and/or padding to input data.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  using fptr = double (*)(const double, const double);
  fptr getApodizationFunction(const std::string &method);
  HistogramData::Histogram applyApodizationFunction(const HistogramData::Histogram &histogram,
                                                    const double decayConstant, fptr function);
  HistogramData::Histogram addPadding(const HistogramData::Histogram &histogram, const int padding);
};

} // namespace Algorithms
} // namespace Mantid
