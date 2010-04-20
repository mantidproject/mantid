#ifndef MANTID_ALGORITHMS_GETEI2_H_
#define MANTID_ALGORITHMS_GETEI2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidGeometry/Instrument/Component.h"
#include <vector>
#include <string>

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

    @author Martyn Gigg STFC Rutherford Appleton Laboratory
    @date 31/03/2010

    Copyright &copy; 2008-9 STFC Rutherford Appleton Laboratory

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
      /// empty contructor calls the base class constructor
      GetEi2() : Algorithm(), m_mon_indices() {}

      /// Initialize the algorithm
      void init();
      /// Execute the algorithm
      void exec();

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "GetEi"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 2; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const{return "CorrectionFunctions";}

    private:
      void advanceProgress(double toAdd);
      void getGeometry(DataObjects::Workspace2D_const_sptr WS, double &monitor0Dist, double &monitor1Dist) const;
      double timeToFly(double s, double E_KE) const;
      double getPeakCentre(API::MatrixWorkspace_const_sptr WS, const int monitIn, const double peakTime);
      void extractSpec(int specInd, double start = -1.0, double end = -1.0);
      double getPeakFirstMoments(API::MatrixWorkspace_sptr WS, const double tMin, const double tMax);
      void regroup(double xmin, double delta, double xmax, const MantidVec &x, const MantidVec &y, const MantidVec &e, MantidVec& xnew, MantidVec& ynew, MantidVec& enew);
      void getPeakMean(const MantidVec& Xs, const MantidVec& Ys, const MantidVec& Es, const double prominence, double &area, double &c, double &c_fwhm, double &w, double &xbar);
      void integrate(double &bkgd_m, double &bkgd_err_m, const MantidVec &x, const MantidVec &y, const MantidVec &e, const double start, const double end);
      API::MatrixWorkspace_sptr smooth(API::MatrixWorkspace_sptr WS);
      API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr WS, const double first, const double width, const double end);
      double neutron_E_At(double speed) const;
      /// An estimate of the percentage of the algorithm runtimes that has been completed 
      double m_fracCompl;
      /// name of the tempory workspace that we create and will contain the monitor histogram that we're examining
      API::MatrixWorkspace_sptr m_tempWS;
      /// Workspace indices for the monitors
      std::vector<int> m_mon_indices;

      // for estimating algorithm progress
      static const double CROP;                                ///< fraction of algorithm time taken up with running CropWorkspace
      static const double GET_COUNT_RATE;                      ///< fraction of algorithm taken by a single call to ConvertToDistribution
      static const double FIT_PEAK;                            ///< fraction required to find a peak

    };

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_GETEI2_H_*/