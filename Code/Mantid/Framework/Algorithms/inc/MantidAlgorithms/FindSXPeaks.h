#ifndef MANTID_ALGORITHMS_FINDSXPEAKS_H_
#define MANTID_ALGORITHMS_FINDSXPEAKS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"


namespace Mantid
{
namespace Algorithms
{
/** Takes a 2D workspace as input and find the FindSXPeaksimum in each 1D spectrum.
    The algorithm creates a new 1D workspace containing all FindSXPeaksima as well as their X boundaries
    and error. This is used in particular for single crystal as a quick way to find strong peaks.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> Range_lower - The X value to search from (default 0)</LI>
    <LI> Range_upper - The X value to search to (default FindSXPeaks)</LI>
    <LI> StartSpectrum - Start spectrum number (default 0)</LI>
    <LI> EndSpectrum - End spectrum number  (default FindSXPeaks)</LI>
    </UL>

    @author L C Chapon, ISIS, Rutherford Appleton Laboratory
    @date 11/08/2009
    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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


struct DLLExport SXPeak
{
 public:
  SXPeak(double t, double th2, double phi,double intensity,const std::vector<int>& spectral,double Ltot, Mantid::detid_t detectorId):_t(t),_th2(th2),_phi(phi),_intensity(intensity),_Ltot(Ltot), _detectorId(detectorId)
	{
    if(intensity < 0)
    {
      throw std::invalid_argument("SXPeak: Cannot have an intensity < 0");
    }
    if(spectral.size() == 0)
    {
      throw std::invalid_argument("SXPeak: Cannot have zero sized spectral list");
    }
    if(Ltot < 0)
    {
      throw std::invalid_argument("SXPeak: Cannot have detector distance < 0");
    }
		npixels=1;
		_spectral.resize(spectral.size());
		std::copy(spectral.begin(),spectral.end(),_spectral.begin());
	}
	bool compare(const SXPeak& rhs, double tolerance) const
	{
		if (std::abs(_t/npixels-rhs._t/rhs.npixels)>tolerance*_t/npixels)
			return false;
		if (std::abs(_phi/npixels-rhs._phi/rhs.npixels)>tolerance*_phi/npixels)
			return false;
		if (std::abs(_th2/npixels-rhs._th2/rhs.npixels)>tolerance*_th2/npixels)
			return false;
		return true;
	}
	Mantid::Kernel::V3D getQ() const
	{
		double Qx= -sin(_th2)*cos(_phi);
		double Qy= -sin(_th2)*sin(_phi);
		double Qz= 1.0 - cos(_th2);
		// Neutron velocity vi ( speed in m/s )
		double vi = _Ltot / (_t*1e-6);
    // wavelength = h / mv
    double wi = PhysicalConstants::h / (PhysicalConstants::NeutronMass * vi);
    // in angstroms
    wi *= 1e10;
    //wavevector=1/wavelength
    double wvi=1.0/wi;
    // Scale the scattered direction by the wavevector
		return Mantid::Kernel::V3D(Qx*wvi,Qy*wvi,Qz*wvi);
	}
	SXPeak& operator+=(const SXPeak& rhs)
	{
		_t+=rhs._t;
		_phi+=rhs._phi;
		_th2+=rhs._th2;
		_intensity+=rhs._intensity;
		_Ltot+=rhs._Ltot;
		npixels+=1;
		for (std::size_t i=0;i<rhs._spectral.size();i++)
			_spectral.push_back(rhs._spectral[i]);
		return *this;
	}
	void reduce()
	{
		_t/=npixels;
		_phi/=npixels;
		_th2/=npixels;
		_intensity/=npixels;
		_Ltot/=npixels;
		npixels=1;
	}
	friend std::ostream& operator<<(std::ostream& os,const SXPeak& rhs)
	{
		os << rhs._t << "," << rhs._th2 << "," << rhs._phi << "," << rhs._intensity << "\n";
		os << " Spectra";
		std::copy(rhs._spectral.begin(),rhs._spectral.end(),std::ostream_iterator<int>(os,","));
		return os;
	}
  const double& getIntensity() const
  {
    return _intensity;
  }
  const Mantid::detid_t& getDetectorId() const
  {
    return _detectorId;
  }
  
private:
  double _intensity;
  const Mantid::detid_t _detectorId;
  static double mN;
	static double hbar;
  double _t;
  double _th2;
  double _phi;
	int npixels;
	std::vector<int> _spectral;
	double _Ltot;
};

typedef std::vector<SXPeak> peakvector;

class DLLExport FindSXPeaks : public API::Algorithm
{
public:
  /// Default constructor
  FindSXPeaks() : API::Algorithm() {};
  /// Destructor
  virtual ~FindSXPeaks() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FindSXPeaks";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction";}

private:
  void initDocs();
  // Overridden Algorithm methods
  void init();
  //
  void exec();
  //
  void reducePeakList(const peakvector&);
  /// The value in X to start the search from
  double m_MinRange;
  /// The value in X to finish the search at
  double m_MaxRange;
  /// The spectrum to start the integration from
  int m_MinSpec;
  /// The spectrum to finish the integration at
  int m_MaxSpec;
  // The peaks workspace that contains the peaks information.
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaks;

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_FindSXPeaks_H_*/
