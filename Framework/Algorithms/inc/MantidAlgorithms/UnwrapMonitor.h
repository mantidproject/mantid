// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {
/** Takes an input Workspace2D that contains 'raw' data, unwraps the data
   according to
    the reference flightpath provided and converts the units to wavelength.
    The output workspace will have common bins in the maximum theoretical
   wavelength range.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    <LI> LRef            - The 'reference' flightpath (in metres). </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 25/07/2008
*/
class MANTID_ALGORITHMS_DLL UnwrapMonitor : public API::Algorithm {
public:
  UnwrapMonitor();
  ~UnwrapMonitor() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "UnwrapMonitor"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Takes an input workspace that contains 'raw' data, unwraps the "
           "data "
           "according to the reference flightpath provided and converts the "
           "units to wavelength."
           "The output workspace will have common bins in the maximum "
           "theoretical wavelength range.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"UnwrapMonitorsInTOF", "UnwrapSNS"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions\\InstrumentCorrections"; }

private:
  void init() override;
  void exec() override;

  const std::vector<int> unwrapX(std::vector<double> &newX, const int &spectrum, const double &Ld);
  std::pair<int, int> handleFrameOverlapped(const Mantid::HistogramData::HistogramX &xdata, const double &Ld,
                                            std::vector<double> &tempX);
  void unwrapYandE(const API::MatrixWorkspace_sptr &tempWS, const int &spectrum, const std::vector<int> &rangeBounds,
                   std::vector<double> &newY, std::vector<double> &newE);
  API::MatrixWorkspace_sptr rebin(const API::MatrixWorkspace_sptr &workspace, const double &min, const double &max,
                                  const size_t &numBins);

  double m_conversionConstant; ///< The constant used in the conversion from TOF
  /// to wavelength
  API::MatrixWorkspace_const_sptr m_inputWS; ///< Pointer to the input workspace
  double m_LRef;                             ///< The 'reference' flightpath
  double m_Tmin;                             ///< The start of the time-of-flight frame
  double m_Tmax;                             ///< The end of the time-of-flight frame
  size_t m_XSize;                            ///< The size of the X vectors in the input workspace
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress = nullptr;
};

} // namespace Algorithms
} // namespace Mantid
