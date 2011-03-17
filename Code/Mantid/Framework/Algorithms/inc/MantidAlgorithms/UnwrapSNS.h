#ifndef MANTID_ALGORITHMS_UNWRAP_H_
#define MANTID_ALGORITHMS_UNWRAP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes an input Workspace2D that contains 'raw' data, unwraps the data according to
    the reference flightpath provided and converts the units to wavelength.
    The output workspace will have common bins in the maximum theoretical wavelength range.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    <LI> LRef            - The 'reference' flightpath (in metres). </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 25/07/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport UnwrapSNS : public API::Algorithm
{
public:
  UnwrapSNS();
  virtual ~UnwrapSNS();
  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  void init();
  void exec();
  void execEvent();

  double calculateFlightpath(const std::size_t& spectrum, bool& isMonitor) const;
  size_t unwrapX(const MantidVec&, MantidVec&, const double& Ld);
  double m_conversionConstant; ///< The constant used in the conversion from TOF to wavelength
  API::MatrixWorkspace_const_sptr m_inputWS; ///< Pointer to the input workspace
  DataObjects::EventWorkspace_const_sptr m_inputEvWS; ///< Pointer to the input event workspace
  double m_LRef; ///< The 'reference' flightpath
  double m_L1; ///< The instrument initial flightpath
  double m_Tmin; ///< The start of the time-of-flight frame
  double m_Tmax; ///< The end of the time-of-flight frame
  double m_frameWidth; ///< The width of the frame cached to speed up things
  std::size_t m_numberOfSpectra; ///< The number of spectra in the workspace
  std::size_t m_XSize; ///< The size of the X vectors in the input workspace
  /// Progress reporting
  API::Progress* m_progress;
};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_UNWRAP_H_ */
