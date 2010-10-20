#ifndef COMPONENTCREATIONHELPERS_H_
#define COMPONENTCREATIONHELPERS_H_

#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

using namespace Mantid::Geometry;
typedef boost::shared_ptr<Object> Object_sptr;

namespace ComponentCreationHelper
{

  /** 
  A set of helper functions for creating various component structures for the unit tests.

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

  static Object_sptr createCappedCylinder(double radius, double height, const V3D & baseCentre, const V3D & axis, const std::string & id)
  {
    std::ostringstream xml;
    xml << "<cylinder id=\"" << id << "\">" 
      << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\"" << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\"/>"
      << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
      << "<radius val=\"" << radius << "\" />"
      << "<height val=\"" << height << "\" />"
      << "</cylinder>";

    ShapeFactory shapeMaker;
    return shapeMaker.createShape(xml.str());
  }

  /**
  * Create a component assembly at the origin made up of 4 cylindrical detectors
  */
  static boost::shared_ptr<CompAssembly> createTestAssemblyOfFourCylinders()
  {
    boost::shared_ptr<CompAssembly> bank = boost::shared_ptr<CompAssembly>(new CompAssembly("BankName"));
    // One object
    Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(0.5, 1.5, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube"); 
    // Four object components
    for( size_t i = 1; i < 5; ++i )
    {
      ObjComponent * physicalPixel = new ObjComponent("pixel", pixelShape);
      physicalPixel->setPos(static_cast<double>(i),0.0,0.0);
      bank->add(physicalPixel);
    }
    
    return bank;
  }

  /**
   * Create a detector group containing 5 detectors
   */
  static boost::shared_ptr<DetectorGroup> createDetectorGroupWith5CylindricalDetectors()
  {
    const int ndets = 5;
    std::vector<boost::shared_ptr<IDetector> > groupMembers(ndets);
    // One object
    Object_sptr detShape = ComponentCreationHelper::createCappedCylinder(0.5, 1.5, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube"); 
    for( int i = 0; i < ndets; ++i )
    {
      std::ostringstream os;
      os << "d" << i;
      boost::shared_ptr<Detector> det(new Detector(os.str(), detShape, NULL));
      det->setID(i+1);
      det->setPos((double)(i+1), 2.0, 2.0);
      groupMembers[i] = det;
    }

    return boost::shared_ptr<DetectorGroup>(new DetectorGroup(groupMembers, false));
  }

  /**
   * Create a group of two monitors
   */
  static boost::shared_ptr<DetectorGroup> createGroupOfTwoMonitors()
  {
    const int ndets(2);
    std::vector<boost::shared_ptr<IDetector> > groupMembers(ndets);
    for( int i = 0; i < ndets; ++i )
    {
      std::ostringstream os;
      os << "m" << i;
      boost::shared_ptr<Detector> det(new Detector(os.str(), NULL));
      det->setID(i+1);
      det->setPos((double)(i+1), 2.0, 2.0);
      det->markAsMonitor();
      groupMembers[i] = det;
    }
    return boost::shared_ptr<DetectorGroup>(new DetectorGroup(groupMembers, false));
  }
}

#endif //COMPONENTCREATIONHELPERS_H_