#ifndef MANTID_ALGORITHMS_CYLINDERABSORPTION_H_
#define MANTID_ALGORITHMS_CYLINDERABSORPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Object.h"
#include <climits>

namespace Mantid
{
namespace Algorithms
{
/** Calculates attenuation due to absorption and scattering in a cylindrical sample.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
    <LI> CylinderSampleHeight - The height of the cylindrical sample in centimetres. </LI>
    <LI> CylinderSampleRadius - The radius of the cylindrical sample in centimetres. </LI>
    <LI> AttenuationXSection - The attenuation cross-section for the sample material in barns. </LI>
    <LI> ScatteringXSection - The scattering cross-section for the sample material in barns. </LI>
    <LI> SampleNumberDensity - The number density of the sample in Angstrom^-3.</LI>
    <LI> NumberOfSlices - The number of slices into which the cylinder is divided for the calculation. </LI>
    <LI> NumberOfAnnuli - The number of annuli into which each slice is divided for the calculation. </LI>
    <LI> NumberOfWavelengthPoints - The number of wavelength points for which numerical integral is calculated (default: all points). </LI>
    <LI> ExpMethod - The method to calculate exponential function (Normal of Fast approximation). </LI>
    </UL>

    This algorithm uses numerical integration method to calculate attenuation factors
    resulting from absorption and single scattering in a cylindrical sample with the dimensions and material
    properties given. Factors are calculated for each spectrum (i.e. detector position) and wavelength point,
    as defined by the input workspace. The sample is divided up into a stack of slices, which are then divided
    into annuli (rings). These annuli are further divided to give the full set of elements for which a calculation
    will be carried out. Thus the calculation speed depends linearly on the total number of bins in the workspace
    and on the number of slices. The dependence on the number of annuli is stronger, going as 3n(n + 1).

    Path lengths through the sample are then calculated for the centre-point of each element and a numerical
    integration is carried out using these path lengths over the volume elements.

    This algorithm assumes that the beam comes along the Z axis, that Y (the sample cylinder axis) is up
    and that the sample is at the origin.

    @author Russell Taylor, Tessella Support Services plc
    @date 02/12/2008

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CylinderAbsorption : public API::Algorithm
{
public:
  /// (Empty) Constructor
  CylinderAbsorption();
  /// Virtual destructor
  virtual ~CylinderAbsorption() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CylinderAbsorption"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void retrieveProperties();
  void constructCylinderSample();
  void initialiseCachedDistances();
  void calculateDistances(const Geometry::IDetector_const_sptr& detector, std::vector<double>& lTotal) const;
  inline double doIntegration(const double& lambda,const std::vector<double>& lTotal) const;
  void interpolate(const MantidVec& x, MantidVec& y, bool is_histogram);
  
  /// Sample object. Keeping separate from the full instrument geometry for now.
  Geometry::Object m_cylinderSample;
  double m_cylHeight;   ///< The height of the cylindrical sample in cm
  double m_cylRadius;   ///< The radius of the cylindrical sample in cm
  double m_refAtten;    ///< The attenuation cross-section in barns at 1.8A
  double m_scattering;  ///< The scattering cross-section in barns
  std::vector<double> m_L1s,  ///< Cached L1 distances
                      m_elementVolumes;  ///< Cached element volumes
  int n_lambda;         ///< The number of points in wavelength, the rest is interpolated linearly
  int x_step;           ///< The step in bin number between adjacent points
	int m_numSlices;				///< The unmber of slices
  double m_sliceThickness;///<  The slice thickness
  int m_numAnnuli;				///< The number of annuli
  double m_deltaR;				///< radius of the cylindrical sample in cm / The number of annuli
  int m_numVolumeElements;///< the number of volume elements

  std::vector<std::string> exp_options; ///< Options to compute exponential function

  typedef double (*expfunction)(double); ///< Typedef pointer to exponential function
  expfunction EXPONENTIAL; ///< Pointer to exponential function
      
  ///a flag int value to indicate that the value wasn't set by users
  static const int unSetInt = INT_MAX-15;


};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CYLINDERABSORPTION_H_*/
