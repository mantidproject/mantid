#ifndef MANTID_CRYSTAL_ANVREDCORRECTION_H_
#define MANTID_CRYSTAL_ANVREDCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Crystal
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

    @author Vickie Lynch, Dennis Mikkelson SNS
    @date 06/14/2011

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
  const double pc[4][19] =
  {    {0.9369, 0.9490, 0.9778, 1.0083, 1.0295, 1.0389, 1.0392, 1.0338,
        1.0261, 1.0180, 1.0107, 1.0046, 0.9997, 0.9957, 0.9929, 0.9909,
        0.9896, 0.9888, 0.9886},
       {2.1217, 2.0149, 1.7559, 1.4739, 1.2669, 1.1606, 1.1382, 1.1724,
        1.2328, 1.3032, 1.3706, 1.4300, 1.4804, 1.5213, 1.5524, 1.5755,
        1.5913, 1.6005, 1.6033},
       {-0.1304, 0.0423, 0.4664, 0.9427, 1.3112, 1.5201, 1.5844, 1.5411,
        1.4370, 1.2998, 1.1543, 1.0131, 0.8820, 0.7670, 0.6712, 0.5951,
        0.5398, 0.5063, 0.4955},
       {1.1717, 1.0872, 0.8715, 0.6068, 0.3643, 0.1757, 0.0446, -0.0375,
       -0.0853, -0.1088, -0.1176, -0.1177, -0.1123, -0.1051, -0.0978,
       -0.0914, -0.0868, -0.0840, -0.0833}};
  const double MAX_WAVELENGTH = 50.0;    // max in lamda_weight table

  const double STEPS_PER_ANGSTROM = 100;  // resolution of lamda table 

  const int NUM_WAVELENGTHS = std::ceil( MAX_WAVELENGTH * STEPS_PER_ANGSTROM);

  const double radtodeg_half = 180.0/M_PI/2.;


class DLLExport AnvredCorrection : public API::Algorithm
{
public:
  /// (Empty) Constructor
  AnvredCorrection();
  /// Virtual destructor
  virtual ~AnvredCorrection() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "AnvredCorrection"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Crystal"; }

protected:
  /** A virtual function in which additional properties of an algorithm should be declared. 
   *  Called by init().
   */
  virtual void defineProperties() { /*Empty in base class*/ }
  /// A virtual function in which additional properties should be retrieved into member variables. 
  virtual void retrieveProperties() { /*Empty in base class*/ }

  API::MatrixWorkspace_sptr m_inputWS;     ///< A pointer to the input workspace
  /// Shared pointer to the event workspace
  DataObjects::EventWorkspace_sptr eventW;

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  void execEvent();
  void cleanup();


  void retrieveBaseProperties();
  void constructSample(API::Sample& sample);
  double getEventWeight( double lamda, double two_theta);
  void BuildLamdaWeights();
  double absor_sphere(double& twoth, double& wl) ;
  void GetSpectrumWeights(std::string spectrum_file_name, std::vector<double> lamda_weight);
  
  double smu; // in 1/cm
  double amu; // in 1/cm
  double radius; // in cm
  double power_th;  //Power of lamda in BuildLamdaWeights
  std::vector<double> lamda_weight;

};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_ANVREDCORRECTION_H_*/
