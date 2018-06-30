#ifndef MANTID_ALGORITHMS_UNWRAPSNS_H_
#define MANTID_ALGORITHMS_UNWRAPSNS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
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

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport UnwrapSNS : public API::Algorithm {
public:
  UnwrapSNS();
  ~UnwrapSNS() override {}
  const std::string name() const override { return "UnwrapSNS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Takes an input workspace that contains 'raw' data, unwraps the "
           "data according to the reference flightpath provided and converts "
           "the units to wavelength. The output workspace will have common "
           "bins in the maximum theoretical wavelength range.";
  }

  int version() const override { return 1; }

  const std::string category() const override {
    return "CorrectionFunctions\\InstrumentCorrections";
  }

private:
  void init() override;
  void exec() override;
  void execEvent();
  void runMaskDetectors();

  int unwrapX(const Mantid::HistogramData::HistogramX &,
              std::vector<double> &dataout, const double &Ld);
  void getTofRangeData(const bool);
  double m_conversionConstant; ///< The constant used in the conversion from TOF
  /// to wavelength
  API::MatrixWorkspace_const_sptr m_inputWS; ///< Pointer to the input workspace
  DataObjects::EventWorkspace_const_sptr
      m_inputEvWS;       ///< Pointer to the input event workspace
  double m_LRef;         ///< The 'reference' flightpath
  double m_Tmin;         ///< The start of the time-of-flight frame
  double m_Tmax;         ///< The end of the time-of-flight frame
  double m_frameWidth;   ///< The width of the frame cached to speed up things
  int m_numberOfSpectra; ///< The number of spectra in the workspace
  int m_XSize;           ///< The size of the X vectors in the input workspace
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_UNWRAPSNS_H_ */
