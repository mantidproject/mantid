#ifndef MANTID_ALGORITHMS_ABSORPTIONCORRECTION_H_
#define MANTID_ALGORITHMS_ABSORPTIONCORRECTION_H_
/*WIKI* 

This algorithm uses a numerical integration method to calculate attenuation factors resulting from absorption and single scattering in a sample with the material properties given. Factors are calculated for each spectrum (i.e. detector position) and wavelength point, as defined by the input workspace. 
The sample is first bounded by a cuboid, which is divided up into small cubes. The cubes whose centres lie within the sample make up the set of integration elements (so you have a kind of 'Lego' model of the sample) and path lengths through the sample are calculated for the centre-point of each element, and a numerical integration is carried out using these path lengths over the volume elements.

Note that the duration of this algorithm is strongly dependent on the element size chosen, and that too small an element size can cause the algorithm to fail because of insufficient memory.

==== Assumptions ====
This algorithm assumes that the (parallel) beam illuminates the entire sample '''unless''' a 'gauge volume' has been defined using the [[DefineGaugeVolume]] algorithm (or by otherwise adding a valid XML string [[HowToDefineGeometricShape | defining a shape]] to a [[Run]] property called "GaugeVolume"). In this latter case only scattering within this volume (and the sample) is integrated, because this is all the detector can 'see'. The full sample is still used for the neutron paths. ('''N.B.''' If your gauge volume is of axis-aligned cuboid shape and fully enclosed by the sample then you will get a more accurate result from the [[CuboidGaugeVolumeAbsorption]] algorithm.)

==== Restrictions on the input workspace ====
The input workspace must have units of wavelength. The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed.



*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** A base class for absorption correction algorithms.

    Common Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
    <LI> AttenuationXSection - The attenuation cross-section for the sample material in barns. </LI>
    <LI> ScatteringXSection - The scattering cross-section for the sample material in barns. </LI>
    <LI> SampleNumberDensity - The number density of the sample in Angstrom^-3.</LI>
    <LI> NumberOfWavelengthPoints - The number of wavelength points for which numerical integral is calculated (default: all points). </LI>
    <LI> ExpMethod - The method to calculate exponential function (Normal of Fast approximation). </LI>
    </UL>

    This class, which must be overridden to provide the specific sample geometry and integration
    elements, uses a numerical integration method to calculate attenuation factors resulting 
    from absorption and single scattering in a sample. Factors are calculated for each spectrum
    (i.e. detector position) and wavelength point, as defined by the input workspace.
    Path lengths through the sample are then calculated for the centre-point of each element 
    and a numerical integration is carried out using these path lengths over the volume elements.

    This algorithm assumes that the beam comes along the Z axis, that Y is up
    and that the sample is at the origin.

    @author Russell Taylor, Tessella plc
    @date 04/02/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport AbsorptionCorrection : public API::Algorithm
{
public:
  /// (Empty) Constructor
  AbsorptionCorrection();
  /// Virtual destructor
  virtual ~AbsorptionCorrection() {}
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Absorption Corrections"; }

protected:
  /** A virtual function in which additional properties of an algorithm should be declared. 
   *  Called by init().
   */
  virtual void defineProperties() { /*Empty in base class*/ }
  /// A virtual function in which additional properties should be retrieved into member variables. 
  virtual void retrieveProperties() { /*Empty in base class*/ }
  /// Returns the XML string describing the sample, which can be used by the ShapeFactory
  virtual std::string sampleXML() = 0;
  /** Calculate the distances for L1 and element size for each element in the sample.
   *  Also calculate element position, assuming sample is at origin (they are shifted in exec if
   *  this is not the case).
   */
  virtual void initialiseCachedDistances() = 0;

  API::MatrixWorkspace_const_sptr m_inputWS;     ///< A pointer to the input workspace
  const Geometry::Object* m_sampleObject;        ///< Local cache of sample object.
  Kernel::V3D m_beamDirection;                 ///< The direction of the beam.
  std::vector<double> m_L1s,                     ///< Cached L1 distances
                      m_elementVolumes;          ///< Cached element volumes
  std::vector<Kernel::V3D> m_elementPositions; ///< Cached element positions
  size_t m_numVolumeElements;                    ///< The number of volume elements
  double m_sampleVolume;                         ///< The total volume of the sample

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  void retrieveBaseProperties();
  void constructSample(API::Sample& sample);
  void calculateDistances(const Geometry::IDetector_const_sptr& detector, std::vector<double>& L2s) const;
  inline double doIntegration(const double& lambda,const std::vector<double>& L2s) const;
  inline double doIntegration(const double& lambda_i,const double& lambda_f,const std::vector<double>& L2s) const;
  
  double m_refAtten;    ///< The attenuation cross-section in 1/m at 1.8A
  double m_scattering;  ///< The scattering cross-section in 1/m
  int64_t n_lambda;     ///< The number of points in wavelength, the rest is interpolated linearly
  int64_t x_step;       ///< The step in bin number between adjacent points
  int64_t m_emode;      ///< The energy mode: 0 - elastic, 1 - direct, 2 - indirect
  double m_lambdaFixed; ///< The wavelength corresponding to the fixed energy, if provided

  typedef double (*expfunction)(double); ///< Typedef pointer to exponential function
  expfunction EXPONENTIAL; ///< Pointer to exponential function

};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ABSORPTIONCORRECTION_H_*/
