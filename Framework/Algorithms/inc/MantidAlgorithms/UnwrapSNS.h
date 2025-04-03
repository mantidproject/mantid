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
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

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
class MANTID_ALGORITHMS_DLL UnwrapSNS final : public API::Algorithm, public API::DeprecatedAlgorithm {
public:
  UnwrapSNS();
  ~UnwrapSNS() override = default;
  const std::string name() const override { return "UnwrapSNS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Takes an input workspace that contains 'raw' data, unwraps the "
           "data according to the reference flightpath provided and converts "
           "the units to wavelength. The output workspace will have common "
           "bins in the maximum theoretical wavelength range.";
  }

  int version() const override { return 1; }

  const std::string category() const override { return "CorrectionFunctions\\InstrumentCorrections"; }

private:
  void init() override;
  void exec() override;
  void execEvent();
  void runMaskDetectors();

  int unwrapX(const Mantid::HistogramData::HistogramX &, std::vector<double> &dataout, const double &Ld);
  void getTofRangeData(const bool);
  double m_conversionConstant; ///< The constant used in the conversion from TOF
  /// to wavelength
  API::MatrixWorkspace_const_sptr m_inputWS;          ///< Pointer to the input workspace
  DataObjects::EventWorkspace_const_sptr m_inputEvWS; ///< Pointer to the input event workspace
  double m_LRef;                                      ///< The 'reference' flightpath
  double m_Tmin;                                      ///< The start of the time-of-flight frame
  double m_Tmax;                                      ///< The end of the time-of-flight frame
  double m_frameWidth;                                ///< The width of the frame cached to speed up things
  int m_numberOfSpectra;                              ///< The number of spectra in the workspace
  int m_XSize;                                        ///< The size of the X vectors in the input workspace
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress;
};

} // namespace Algorithms
} // namespace Mantid
