#ifndef COMPONENTCREATIONHELPER_H_
#define COMPONENTCREATIONHELPER_H_

#include "MantidTestHelpers/DLLExport.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"

// Forward declarations
namespace Mantid
{
  namespace Geometry
  {
    class CompAssembly;
    class ObjComponent;
    class DetectorGroup;
    class DetectorsRing;
  }
}

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


  //----------------------------------------------------------------------------------------------
  /**
   * Create a capped cylinder object
   */
  DLL_TESTHELPERS Mantid::Geometry::Object_sptr 
  createCappedCylinder(double radius, double height, const Mantid::Geometry::V3D & baseCentre, 
               const Mantid::Geometry::V3D & axis, const std::string & id);
  /**
   * Return the XML for a sphere.
   */
  DLL_TESTHELPERS std::string sphereXML(double radius, const Mantid::Geometry::V3D & centre, const std::string & id);
  /**
   * Create a sphere object
   */
  DLL_TESTHELPERS Mantid::Geometry::Object_sptr createSphere(double radius, const Mantid::Geometry::V3D & centre, const std::string & id);
  /** Create a cuboid shape for your pixels */
  DLL_TESTHELPERS Mantid::Geometry::Object_sptr createCuboid(double x_side_length, double y_side_length = -1.0, 
                                 double z_side_length = -1.0);
  /**
  * Create a component assembly at the origin made up of 4 cylindrical detectors
  */
  DLL_TESTHELPERS boost::shared_ptr<Mantid::Geometry::CompAssembly> createTestAssemblyOfFourCylinders();
  /**
   * Create an object component that has a defined shape
   */
  DLL_TESTHELPERS Mantid::Geometry::ObjComponent * createSingleObjectComponent();
  /**
   * Create a hollow shell, i.e. the intersection of two spheres or radius r1 and r2
   */
  DLL_TESTHELPERS Mantid::Geometry::Object_sptr createHollowShell(double innerRadius, double outerRadius, 
                        const Mantid::Geometry::V3D & centre = Mantid::Geometry::V3D());
  /**
   * Create a detector group containing 5 detectors
   */
  DLL_TESTHELPERS boost::shared_ptr<Mantid::Geometry::DetectorGroup> createDetectorGroupWith5CylindricalDetectors();
   /**
   * Create a detector group containing n detectors with gaps
   */
  DLL_TESTHELPERS boost::shared_ptr<Mantid::Geometry::DetectorGroup> createDetectorGroupWithNCylindricalDetectorsWithGaps(unsigned int nDet=4,double gap=0.01);
  /**
   * Create a detector group containing detectors ring
   * R_min -- min radius of the ring
   * R_max -- max radius of the ring, center has to be in 0 position,
   * z     -- axial z-coordinate of the detectors position; 
    The detectors are the cylinders with 1.5cm height and 0.5 cm radius
   */
  DLL_TESTHELPERS boost::shared_ptr<Mantid::Geometry::DetectorGroup> createRingOfCylindricalDetectors(const double R_min=4.5, const double R_max=5, const double z=4);
  /**
   * Create a group of two monitors
   */
  DLL_TESTHELPERS boost::shared_ptr<Mantid::Geometry::DetectorGroup> createGroupOfTwoMonitors();
  /**
   * Create an test instrument with n panels of 9 cylindrical detectors, a source and spherical sample shape.
   *
   * @param num_banks: number of 9-cylinder banks to create
   * @param verbose: prints out the instrument after creation.
   */
  DLL_TESTHELPERS Mantid::Geometry::IInstrument_sptr 
  createTestInstrumentCylindrical(int num_banks, bool verbose = false);
  /**
   * Create a test instrument with n panels of rectangular detectors, pixels*pixels in size, a source and spherical sample shape.
   *
   * @param num_banks: number of 9-cylinder banks to create
   * @param verbose: prints out the instrument after creation.
   */
  DLL_TESTHELPERS Mantid::Geometry::IInstrument_sptr createTestInstrumentRectangular(int num_banks, int pixels, double pixelSpacing = 0.008);

}

// SANS helpers

class DLL_TESTHELPERS SANSInstrumentCreationHelper
{
public:

  // Number of detector pixels in each dimension
  static const int nBins;
  // The test instrument has 2 monitors
  static const int nMonitors;

  /*
   * Generate a SANS test workspace, with instrument geometry.
   * The geometry is the SANSTEST geometry, with a 30x30 pixel 2D detector.
   *
   * @param workspace: name of the workspace to be created.
   */
  static Mantid::DataObjects::Workspace2D_sptr createSANSInstrumentWorkspace(std::string workspace);
  /** Run the sub-algorithm LoadInstrument (as for LoadRaw)
   * @param inst_name :: The name written in the Nexus file
   * @param workspace :: The workspace to insert the instrument into
   */
  static void runLoadInstrument(const std::string & inst_name,
                Mantid::DataObjects::Workspace2D_sptr workspace);

  /**
   * Populate spectra mapping to detector IDs
   *
   * @param workspace: Workspace2D object
   * @param nxbins: number of bins in X
   * @param nybins: number of bins in Y
   */
  static void runLoadMappingTable(Mantid::DataObjects::Workspace2D_sptr workspace, 
                  int nxbins, int nybins);

};




#endif //COMPONENTCREATIONHELPERS_H_
