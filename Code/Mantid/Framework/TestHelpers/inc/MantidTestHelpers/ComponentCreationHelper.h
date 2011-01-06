#ifndef COMPONENTCREATIONHELPER_H_
#define COMPONENTCREATIONHELPER_H_

#include "MantidTestHelpers/DLLExport.h"

#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

namespace ComponentCreationHelper
{

using namespace Mantid;
using namespace Mantid::Geometry;

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


  //----------------------------------------------------------------------------------------------
  /**
   * Create a capped cylinder object
   */
  DLL_TESTHELPERS Object_sptr createCappedCylinder(double radius, double height, const V3D & baseCentre, const V3D & axis, const std::string & id);
  /**
   * Return the XML for a sphere.
   */
  DLL_TESTHELPERS std::string sphereXML(double radius, const V3D & centre, const std::string & id);
  /**
   * Create a sphere object
   */
  DLL_TESTHELPERS Object_sptr createSphere(double radius, const V3D & centre, const std::string & id);
  /** Create a cuboid shape for your pixels */
  DLL_TESTHELPERS Object_sptr createCuboid(double x_side_length, double y_side_length = -1.0, double z_side_length = -1.0);
  /**
  * Create a component assembly at the origin made up of 4 cylindrical detectors
  */
  DLL_TESTHELPERS boost::shared_ptr<CompAssembly> createTestAssemblyOfFourCylinders();
  /**
   * Create an object component that has a defined shape
   */
  DLL_TESTHELPERS ObjComponent * createSingleObjectComponent();
  /**
   * Create a hollow shell, i.e. the intersection of two spheres or radius r1 and r2
   */
  DLL_TESTHELPERS Object_sptr createHollowShell(double innerRadius, double outerRadius, const V3D & centre = V3D());
  /**
   * Create a detector group containing 5 detectors
   */
  DLL_TESTHELPERS boost::shared_ptr<DetectorGroup> createDetectorGroupWith5CylindricalDetectors();
  /**
   * Create a group of two monitors
   */
  DLL_TESTHELPERS boost::shared_ptr<DetectorGroup> createGroupOfTwoMonitors();
  /**
   * Create an test instrument with n panels of 9 cylindrical detectors, a source and spherical sample shape.
   *
   * @param num_banks: number of 9-cylinder banks to create
   * @param verbose: prints out the instrument after creation.
   */
  DLL_TESTHELPERS IInstrument_sptr createTestInstrumentCylindrical(int num_banks, bool verbose = false);
  /**
   * Create an test instrument with n panels of rectangular detectors, pixels*pixels in size, a source and spherical sample shape.
   *
   * @param num_banks: number of 9-cylinder banks to create
   * @param verbose: prints out the instrument after creation.
   */
  DLL_TESTHELPERS IInstrument_sptr createTestInstrumentRectangular(int num_banks, int pixels);

}

#endif //COMPONENTCREATIONHELPERS_H_
