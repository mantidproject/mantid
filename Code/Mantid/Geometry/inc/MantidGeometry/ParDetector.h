#ifndef MANTID_GEOMETRY_PARDETECTOR_H_
#define MANTID_GEOMETRY_PARDETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/ParObjComponent.h"
#include <string>

namespace Mantid
{
namespace Geometry
{
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Detector;

/** An extension of the ObjectComponent class to add a detector id.

  @author Roman Tolchenov, Tessella Support Services plc

  Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
class DLLExport ParDetector : public ParObjComponent, public IDetector
{
public:
  ///A string representation of the component type
	virtual std::string type() const {return "ParDetectorComponent";}
	ParDetector(const Detector* base, const ParameterMap* map);
	virtual ~ParDetector();
	virtual IComponent* clone() const {return new ParDetector(*this);}
	void setID(int);

	// IDetector methods
    int getID() const;
    V3D getPos() const;
    double getDistance(const IComponent& comp) const;
    double getTwoTheta(const V3D& observer, const V3D& axis) const;
    double getPhi() const;
    double solidAngle(const V3D& observer) const; 
    bool isMasked() const;
    bool isMonitor() const;
    bool isValid(const V3D& point) const;
    /// Returns a reference to itself
    IComponent* getComponent(){return static_cast<IComponent*>(this);}
    // end IDetector methods
    void markAsMonitor(const bool flag = true);

    /** @name Access to parameterized values for Python. */
    //@{
    /**
     * Return the value, as a double, of the named parameter for this detector. If no value is found then an empty
     * vector is returned
     * @param param_name The name of the parameter value
     */
    virtual std::vector<double> getNumberParameter(const std::string & param_name) const;
    
    /**
     * Return the value, as a postion vector, of the named parameter for this detector. If no value is found then an empty
     * vector is returned
     * @param param_name The name of the parameter value
     */
    virtual std::vector<V3D> getPositionParameter(const std::string & param_name) const;

    /**
     * Return the value, as a postion vector, of the named parameter for this detector. If no value is found then an empty
     * vector is returned
     * @param param_name The name of the parameter value
     */
    virtual std::vector<Quat> getRotationParameter(const std::string & param_name) const;
    //@}

 private:
    /**
     * A templated utility function for getting various parameter types and checking if they exist
     */
    template <class TYPE>
    std::vector<TYPE> getParameter(const std::string & p_name) const
    {
      Parameter_sptr param = m_map->get(this, p_name);
      if( param != Parameter_sptr() )
      {
	return std::vector<TYPE>(1, param->value<TYPE>());
      }
      else
      {
	return std::vector<TYPE>(0);
      }
    }

protected:
  ParDetector(const ParDetector&);

private:
  /// Private, unimplemented copy assignment operator
  ParDetector& operator=(const ParDetector&);

};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_PARDETECTOR_H_*/
