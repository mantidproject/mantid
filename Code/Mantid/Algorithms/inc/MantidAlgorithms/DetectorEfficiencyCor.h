#ifndef MANTID_ALGORITHM_DETECTEFFICIENCYCOR_H_
#define MANTID_ALGORITHM_DETECTEFFICIENCYCOR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/V3D.h"
#include <climits>
#include <string>
#include <vector>
#include <map>

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
class DLLExport DetectorEfficiencyCor : public API::Algorithm
{
 public:
  DetectorEfficiencyCor();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "DetectorEfficiencyCor"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const{return "CorrectionFunctions";}

 private:
  /// Retrieve algorithm properties
  void retrieveProperties();
  /// Correct the given spectra index for efficiency
  void correctForEfficiency(int spectraIndex);
  /// Calculate one over the wave vector for 2 bin bounds
  double calculateOneOverK(double loBinBound, double uppBinBound) const;
  /// Sets the detector geometry cache if necessary
  void getDetectorGeometry(boost::shared_ptr<Geometry::IDetector> det, double & detRadius, Geometry::V3D & detAxis);
  /// Computes the distance to the given shape from a starting point
  double distToSurface(const Geometry::V3D start, const Geometry::Object *shape) const;
  /// Computes the detector efficiency for a given paramater
  double detectorEfficiency(const double alpha) const;
  /// Computes an approximate expansion of a Chebysev polynomial
  double chebevApprox(double a, double b, const double exspansionCoefs[], double x) const;
  /// Log any errors with spectra that occurred
  void logErrors() const;


private:
  /// the user selected workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;
  /// points the map that stores additional properties for detectors in that map
  const Geometry::ParameterMap *m_paraMap;

  /// stores the user selected value for incidient energy of the neutrons
  double m_Ei;
  /// stores the wave number of incidient neutrons, calculated from the energy
  double m_ki;

  /// A lookup of previously seen shape objects used to save calculation time as most detectors have the same shape
  std::map<const Geometry::Object *, std::pair<double, Geometry::V3D> > m_shapeCache;
  /// Sample position
  Geometry::V3D m_samplePos;
  /// The spectra numbers that were skipped
  std::vector<int> m_spectraSkipped;


  // Implement abstract Algorithm methods
  void init();
  void exec();

  ///// Links the energy to the wave number, I got this from Prof T.G.Perring 
  //const double KSquaredToE;
  ///// coefficients for Taylor series/assymptotic expansion used at large wavenumbers and large angle
  //const double c_eff_f[];
  ///// coefficients for Taylor series/assymptotic expansion used at low wavenumbers and low angle
  //const double c_eff_g[];
  ///// the number of coefficients in each of the c_eff_f and c_eff_g arrays
  //const short NUMCOEFS;
  ///// constants to use for the ISIS 3He detectors 2.0*sigref*wref/atmref
  //const double g_helium_prefactor;
  
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DETECTEFFICIENCYCOR_H_*/
