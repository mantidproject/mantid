#ifndef MANTID_ALGORITHMS_UNWRAPMONITOR_H_
#define MANTID_ALGORITHMS_UNWRAPMONITOR_H_

#include "MantidAPI/Algorithm.h"
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
class DLLExport UnwrapMonitor : public API::Algorithm {
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
  const std::vector<std::string> seeAlso() const override {
    return {"UnwrapMonitorsInTOF", "UnwrapSNS"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "CorrectionFunctions\\InstrumentCorrections";
  }

private:
  void init() override;
  void exec() override;

  const std::vector<int> unwrapX(std::vector<double> &newX, const int &spectrum,
                                 const double &Ld);
  std::pair<int, int>
  handleFrameOverlapped(const Mantid::HistogramData::HistogramX &xdata,
                        const double &Ld, std::vector<double> &tempX);
  void unwrapYandE(const API::MatrixWorkspace_sptr &tempWS, const int &spectrum,
                   const std::vector<int> &rangeBounds,
                   std::vector<double> &newY, std::vector<double> &newE);
  API::MatrixWorkspace_sptr rebin(const API::MatrixWorkspace_sptr &workspace,
                                  const double &min, const double &max,
                                  const size_t &numBins);

  double m_conversionConstant; ///< The constant used in the conversion from TOF
  /// to wavelength
  API::MatrixWorkspace_const_sptr m_inputWS; ///< Pointer to the input workspace
  double m_LRef;                             ///< The 'reference' flightpath
  double m_Tmin;  ///< The start of the time-of-flight frame
  double m_Tmax;  ///< The end of the time-of-flight frame
  size_t m_XSize; ///< The size of the X vectors in the input workspace
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress = nullptr;
};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_UNWRAPMONITOR_H_ */
