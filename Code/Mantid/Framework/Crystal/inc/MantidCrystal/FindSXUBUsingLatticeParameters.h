#ifndef MANTID_ALGORITHMS_FindSXUBUsingLatticeParameters_H_
#define MANTID_ALGORITHMS_FindSXUBUsingLatticeParameters_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <boost/tuple/tuple.hpp>
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/V3D.h"
#include <set>

namespace Mantid
{
namespace Crystal
{
/** Takes a 2D workspace as input and find the FindSXUBUsingLatticeParametersimum in each 1D spectrum.
    The algorithm creates a new 1D workspace containing all FindSXUBUsingLatticeParametersima as well as their X boundaries
    and error. This is used in particular for single crystal as a quick way to find strong peaks.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> Range_lower - The X value to search from (default 0)</LI>
    <LI> Range_upper - The X value to search to (default FindSXUBUsingLatticeParameters)</LI>
    <LI> StartSpectrum - Start spectrum number (default 0)</LI>
    <LI> EndSpectrum - End spectrum number  (default FindSXUBUsingLatticeParameters)</LI>
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

struct DLLExport index
{
	index(int h,int k, int l):_h(h),_k(k),_l(l)
	{}
	friend bool operator<(const index& i1, const index& i2)
	{
		if (i1._h<i2._h)
			return true;
		else if (i1._h>i2._h)
			return false;
		if (i1._k<i2._k)
			return true;
		else if (i1._k>i2._k)
			return false;
		if (i1._l<i2._l)
			return true;
		else if (i1._l>i2._l)
			return false;
		return false;
	}
	friend bool operator==(const index& i1, const index& i2)
	{
		if (i1._h!=i2._h)
			return false;
		if (i1._k!=i2._k)
			return false;
		if (i1._l!=i2._l)
			return false;
		return true;

	}
	friend std::ostream& operator<<(std::ostream& os, const index& rhs)
	{
		os << rhs._h << "," << rhs._k << "," << rhs._l;
		return os;
	}
	int _h,_k,_l;
};

class DLLExport PeakCandidate
{
public:
	PeakCandidate(double qx,double qy, double qz):_Q(qx,qy,qz)
	{
	}
	double getdSpacing() const
	{
		return 2*M_PI/_Q.norm();
	}
	void addHKL(int h, int k, int l)
	{
		_hkls.insert(index(h,k,l));
	}
  Mantid::Kernel::V3D getHKL() const
  {
    using namespace Mantid::Kernel;
    if(_hkls.size() != 1)
    {
      throw std::logic_error("Expecting a single HKL value for each peak. Refinement incomplete.");
    }
    V3D result = V3D(_hkls.begin()->_h, _hkls.begin()->_k, _hkls.begin()->_l);
    return result;
  }
	void delHKL(int h, int k, int l)
	{
		std::set<index>::const_iterator it=std::find(_hkls.begin(),_hkls.end(),index(h,k,l));
		if (it!=_hkls.end())
			_hkls.erase(it);
	}
	const Mantid::Kernel::V3D& getQ() const
	{
		return _Q;
	}
	double angle(const PeakCandidate& rhs)
	{
    //Mantid::Kernel::V3D angle = rhs.getQ();
    //return angle.angle(this->getQ());
    return rhs._Q.angle(_Q);
	}
	void setIndex(const std::set<index>& s)
	{
		_hkls.clear();
		_hkls=s;
	}
	void setFirst()
	{
		std::set<index>::iterator it=_hkls.begin(); //Take the first possiblity
		it++;
		_hkls.erase(it,_hkls.end()); //Erase all others!
	}
	void clean(PeakCandidate& rhs,const Mantid::Geometry::UnitCell& uc, double tolerance)
	{
		double measured_angle=this->angle(rhs);
		std::set<index>::iterator it1,it2;
		std::set<index> s1;
		std::set<index> s2;
		for (it1=_hkls.begin();it1!=_hkls.end();it1++) //All possible vectors for hkl on current instance
		{
			for (it2=rhs._hkls.begin();it2!=rhs._hkls.end();it2++) //All possible vectors for hkl on other
			{
				const index& index1=*it1;
				const index& index2=*it2;
				double angle=uc.recAngle(index1._h,index1._k,index1._l,index2._h,index2._k,index2._l,1); //calculate angle between each fictional primative vector on both this and other.
				if (std::abs(angle-measured_angle)<tolerance) //If peak angles are the same as the dspacing angles we can say that this peak corresponds to privatve vector hkl and the other corresponds to primative vector hkl
				{
					s1.insert(index1);
					s2.insert(index2);
				}
			}
		}
		setIndex(s1);
		rhs.setIndex(s2);
	}
	void select(int h, int k, int l)
	{

	}
	friend std::ostream& operator<<(std::ostream& os,const PeakCandidate& rhs)
	{
		os << "Peak" << rhs._Q[0] << "," << rhs._Q[1] << "," << rhs._Q[2] <<  "\n";
		std::copy(rhs._hkls.begin(),rhs._hkls.end(),std::ostream_iterator<index>(os,":"));
		return os;
	}
private:
	Mantid::Kernel::V3D _Q;
	std::set<index> _hkls;
};

class DLLExport FindSXUBUsingLatticeParameters : public API::Algorithm
{
public:
  /// Default constructor
  FindSXUBUsingLatticeParameters() : API::Algorithm() {};
  /// Destructor
  virtual ~FindSXUBUsingLatticeParameters() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FindSXUBUsingLatticeParameters";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction";}

private:

  //Helper method to cull potential hkls off each peak.
  void cullHKLs(int npeaks, std::vector<PeakCandidate>& peaksCandidates, Mantid::Geometry::UnitCell& unitcell);

  void initDocs();
  // Overridden Algorithm methods
  void init();
  //
  void exec();
  //

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_FindSXUBUsingLatticeParameters_H_*/
