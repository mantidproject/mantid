#ifndef MANTID_ALGORITHM_DETECTEFFICIENCYCOR_H_
#define MANTID_ALGORITHM_DETECTEFFICIENCYCOR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/InputWSDetectorInfo.h"
#include "MantidGeometry/Instrument/Component.h"
#include <climits>
#include <string>
#include <vector>

namespace Mantid
{
namespace Algorithms
{
/** 
  Returns efficiency of cylindrical helium gas tube.
    wvec      Final neutron wavevector (Angsstrom^-1)
    rad       Outer radius of cylinder (m)
    atms      Pressure in number of atmospheres of 3He 
    t2rad     Ratio of thickness of tube wall to the
             radius
    sintheta  Sine of the angle between the cylinder
             axis and the directin of travel of the 
             neutron i.e. sintheta=1.0d0 when 
             neutron hits the detector perpendicular
             to the cylinder axis.

   
  T.G.Perring June 1990:
  
  Algorithm is based on a combinateion of Taylor series and
  assymptotic expansion of the double integral for the
  efficiency, linearly interpolating betweent the two in 
  region of common accuracy. Checked against numerical
  integration to yield relative accuracy of 1 part in 10^12
  or better over the entire domain of the input arguments

  T.G.Perring August 2009:
  
  Added generalisation to allow for arbitrary direction of
  path of neutron with respect to the cylinder.


 Origin of data for 3He cross-section
 -------------------------------------
  CKL data : (Argonne)
   "At 2200 m/s xsect=5327 barns    En=25.415 meV         "
   "At 10 atms, rho_atomic=2.688e-4,  so sigma=1.4323 cm-1"

  These data are not quite consistent, but the errors are small :
    2200 m/s = 25.299 meV
    5327 barns & 1.4323 cm-1 ==> 10atms ofideal gas at 272.9K
   but at what temperature are the tubes "10 atms" ?

  Shall use  1.4323 cm-1 @ 3.49416 A-1 with sigma prop. 1/v

  This corresponds to a reference energy of 25.299meV, NOT 25.415.
 This accounts for a difference of typically 1 pt in 1000 for
 energies around a few hundred meV.


    @author Steve Williams based on code by T.G.Perring
    @date 6/10/2009

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
class DLLExport DetectorEfficiencyCor : public API::Algorithm
{
public:
  DetectorEfficiencyCor();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "DetectorEfficiencyCor"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const{return "CorrectionFunctions";}

private:
  /// the user selected workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;
  /// points the map that stores additional properties for detectors in that map
  const Geometry::ParameterMap *m_paraMap;
  /// points to a detector masking helper object, or NULL if the mask map couldn't be opened
  boost::shared_ptr<InputWSDetectorInfo> m_detMasking;

  /// stores the user selected value for incidient energy of the neutrons
  double m_Ei;
  /// stores the wave number of incidient neutrons, calculated from the energy
  double m_ki;

  // lots of cached data
  /// a cached pointer to the shape used to save calculation time as most detectors have the same shape
  const Geometry::Object *m_shapeCache;
  /// the cached value for the radius in the cached shape used to save calculation time
  double m_radCache;
  /// sin of angle between its axis and a line to the sample.
  double m_sinThetaCache;
  /// cached value for the axis of cylinder that the detectors are based on, last time it was calculated
  Geometry::V3D m_baseAxisCache;
  /// cached value a parameter used in the detector efficiency calculation, 1-wallthickness/radius
  double m_1_t2rad;
  /// caches the part of the calculation that is constant for a whole detector
  double m_CONST_rad_sintheta_1_t2rad_atms;

  /// stores 1/wvec for all the bin boundries 
  std::vector<double> m_1_wvec;
  /// points to the start of the workspace X-values, TOF bin boundaries, and is used to tell if the bin boundaries have changed
  const std::vector<double> *m_XsCache;

  ///a flag int value to indicate that the value wasn't set by users
  static const int UNSETINT = INT_MAX-15;

  void retrieveProperties();
  void efficiencyCorrect(int spectraNumber);
  void set1_wvec(int spectraIn);
  double get1OverK(double DeltaE) const;
  void getDetectorGeometry(boost::shared_ptr<Geometry::IDetector> det);
  void getCylinderAxis();
  double DistToSurface(const Geometry::V3D start, const Geometry::Object *shape) const;
  double EFF(const double oneOverwvec) const;//, double rad, double atms, double t2rad, double sintheta);
  double EFFCHB(double a, double b, const double exspansionCoefs[], double x) const;
  void logErrors(std::vector<int> &spuriousSpectra, std::vector<int> &unsetParams) const;

  // Implement abstract Algorithm methods
  void init();
  void exec();

  /// Links the energy to the wave number, I got this from Prof T.G.Perring 
  static const double KSquaredToE;
  /// coefficients for Taylor series/assymptotic expansion used at large wavenumbers and large angle
  static const double c_eff_f[];
  /// coefficients for Taylor series/assymptotic expansion used at low wavenumbers and low angle
  static const double c_eff_g[];
  /// the number of coefficients in each of the c_eff_f and c_eff_g arrays
  static const short NUMCOEFS;
  /// constants to use for the ISIS 3He detectors 2.0*sigref*wref/atmref
  static const double CONSTA;
  
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DETECTEFFICIENCYCOR_H_*/
