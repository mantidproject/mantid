#ifndef MANTID_ALGORITHMS_GETEI2_H_
#define MANTID_ALGORITHMS_GETEI2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace Algorithms
  {
/** Requires an estimate for the initial neutron energy which it uses to
  search for monitor peaks and from these calculate an accurate energy

    Required Properties:
    <UL>
    <LI>InputWorkspace - The X units of this workspace must be time of flight with times in micro-seconds</LI>
    <LI>Monitor1ID - The detector ID of the first monitor</LI>
    <LI>Monitor2ID - The detector ID of the second monitor</LI>
    <LI>EnergyEstimate - An approximate value for the typical incident energy, energy of neutrons leaving the source (meV)</LI>
    <LI>IncidentEnergy - The calculated energy</LI>
    </UL>

    @author Martyn Gigg ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
    @date 31/03/2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
    class DLLExport GetEi2 : public API::Algorithm
    {
    public:
      /// Default constructor
      GetEi2();
      /// Initialize the algorithm
      void init();
      /// Execute the algorithm
      void exec();

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "GetEi"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 2; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const{return "CorrectionFunctions";}

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Calculate Ei from the initial guess given
      double calculateEi(const double initial_guess);
      /// Get the distance from the source of the detector at the workspace index given
      double getDistanceFromSource(const int ws_index) const;
      /// Calculate the peak position within the given window
      double calculatePeakPosition(const int ws_index, const double t_min, const double t_max);
      /// Extract the required spectrum from the input workspace
      API::MatrixWorkspace_sptr extractSpectrum(const int ws_index, const double start, const double end);
      /// Calculate peak width
      double calculatePeakWidthAtHalfHeight(API::MatrixWorkspace_sptr data_ws, const double prominence,
                                            std::vector<double> & peak_x, std::vector<double> & peak_y, std::vector<double> & peak_e) const;
      /// Calculate the value of the first moment of the given spectrum
      double calculateFirstMoment(API::MatrixWorkspace_sptr monitor_ws, const double prominence);
      /// Rebin the given workspace using the given parameters
      API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr monitor_ws, const double first, const double width, const double end);
      /// Integrate the point data
      void integrate(double &integral_value, double &integral_err, const MantidVec &x, const MantidVec &s, const MantidVec &e, const double xmin, const double xmax) const;
      /// Store the incident energy within the sample object
      void storeEi(const double ei) const;

      /// The input workspace
      API::MatrixWorkspace_sptr m_input_ws;
      /// The calculated position of the first peak
      std::pair<int, double> m_peak1_pos;
      /// True if the Ei should be fixed at the guess energy
      bool m_fixedei;
      /// Conversion factor between time and energy
      double m_t_to_mev; 
      /// The percentage deviation from the estimated peak time that defines the peak region 
      const double m_tof_window;
      /// Number of std deviations to consider a peak
      const double m_peak_signif;
      /// Number of std deviations to consider a peak for the derivative
      const double m_peak_deriv;
      /// The fraction of the peak width for the new bins
      const double m_binwidth_frac;
      /// The fraction of the peakwidth to use in calculating background points
      const double m_bkgd_frac;
    };

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_GETEI2_H_*/
