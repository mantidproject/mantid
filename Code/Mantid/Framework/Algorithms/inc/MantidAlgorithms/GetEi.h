#ifndef MANTID_DATAHANDLING_GETEI_H_
#define MANTID_DATAHANDLING_GETEI_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

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

    @author Steve Williams ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
    @date 27/07/2009

    Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport GetEi : public API::Algorithm
{
public:
  GetEi();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GetEi"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const{return "CorrectionFunctions";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// name of the tempory workspace that we create and use
  API::MatrixWorkspace_sptr m_tempWS;
  /// An estimate of the percentage of the algorithm runtimes that has been completed 
  double m_fracCompl;
  /// used by the function findHalfLoc to indicate whether to search left or right
  enum direction
  { GO_LEFT = -1,                                          ///< flag value to serch left
    GO_RIGHT = 1                                           ///< flag value to search right
  };

  // Implement abstract Algorithm methods
  void init();
  void exec();
  
  void getGeometry(API::MatrixWorkspace_const_sptr WS, int64_t mon0Spec, int64_t mon1Spec, double &monitor0Dist, double &monitor1Dist) const;
  std::vector<size_t> getMonitorSpecIndexs(API::MatrixWorkspace_const_sptr WS, int64_t specNum1, int64_t specNum2) const;
  double timeToFly(double s, double E_KE) const;
  double getPeakCentre(API::MatrixWorkspace_const_sptr WS, const int64_t monitIn, const double peakTime);
  void extractSpec(int64_t specInd, double start, double end);
  void getPeakEstimates(double &height, int64_t &centreInd, double &background) const;
  double findHalfLoc(MantidVec::size_type startInd, const double height, const double noise, const direction go) const;
  double neutron_E_At(double speed) const;
  void advanceProgress(double toAdd);

  /// the range of TOF X-values over which the peak will be searched is double this value, i.e. from the estimate of the peak position the search will go forward by this fraction and back by this fraction 
  static const double HALF_WINDOW;
  /// ignore an peaks that are less than this factor of the background
  static const double PEAK_THRESH_H;
  /// ignore peaks where the half width times the ratio of the peak height to the background is less this 
  static const double PEAK_THRESH_A;
  /// for peaks where the distance to the half heigth is less than this number of bins in either direction e.g. the FWHM is less than twice this number
  static const int64_t PEAK_THRESH_W;

  // for estimating algorithm progress
  static const double CROP;                                ///< fraction of algorithm time taken up with running CropWorkspace
  static const double GET_COUNT_RATE;                      ///< fraction of algorithm taken by a single call to ConvertToDistribution
  static const double FIT_PEAK;                            ///< fraction required to find a peak
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_GETEI_H_*/
