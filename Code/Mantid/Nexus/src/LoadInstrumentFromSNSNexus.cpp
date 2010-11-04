//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadInstrumentFromSNSNexus.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>
#include <sstream>
#include <boost/algorithm/string/replace.hpp>


namespace Mantid
{
namespace NeXus
{

// DECLARE_ALGORITHM(LoadInstrumentFromSNSNexus)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadInstrumentFromSNSNexus::LoadInstrumentFromSNSNexus()
{}

/// Initialisation method.
void LoadInstrumentFromSNSNexus::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<Workspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace in which to attach the imported instrument" );

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
		  "The name (including its full or relative path) of the Nexus file to\n"
		  "attempt to load the instrument from. The file extension must either be\n"
		  ".nxs or .NXS" );
}

/** Loads the instrument from the SNS file
 *  @param localWS The workspace
 *  @param entry The entry in the Nexus file
 */
void LoadInstrumentFromSNSNexus::loadInstrument(API::Workspace_sptr localWS,
                            NXEntry entry)

{

    // Here we go through all the entries in the NXS file to list the banks and monitors, and save them to 2 sets of strings
    std::set<std::string,CompareBanks> banks;
    std::set<std::string> monitors;

    for(std::vector<NXClassInfo>::const_iterator it=entry.groups().begin();it!=entry.groups().end();it++)
    if (it->nxclass == "NXdata") // Count detectors
    {
        banks.insert(it->nxname); // sort the bank names
    }
    else if (it->nxclass == "NXmonitor") // Count monitors
    {
        monitors.insert(it->nxname);
    }

    //Cast to the local workspace.
    DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(localWS);

    // Clear off any existing instrument for this workspace
    ws->setInstrument(boost::shared_ptr<Instrument>(new Instrument));

    // Get reference to Instrument and set its name
    boost::shared_ptr<Instrument> instrument = ws->getBaseInstrument();

    std::string xmlSphere = "<sphere id=\"some-shape\">"
             "<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" />"
             "<radius val=\"0.03\" />"
             "</sphere>";

    Geometry::ShapeFactory shapeCreator;
    boost::shared_ptr<Geometry::Object> sphere = shapeCreator.createShape(xmlSphere);

    //Sample is at 0,0,0 (center) by definition.
    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample",sphere,instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    samplepos->setPos(0.0,0.0,0.0);

    //Create the source. We will need to find its position next.
    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",sphere,instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);

    NXInstrument nxInstr = entry.openNXInstrument("instrument");
    instrument->setName(nxInstr.getString("name"));

    //L1 is the moderator distance.
    double l1;
    // If user has provided an L1, use that
    if ( ! Kernel::ConfigService::Instance().getValue("instrument.L1", l1) )
    {
        try // check if moderator/distance exists
        {
            NXFloat nxL1 = nxInstr.openNXFloat("moderator/distance");
            nxL1.load();
            l1 = *nxL1();
            if (m_L1 == 0.) m_L1 = l1;
        }
        catch(...) // hope that m_L1 has been saved earlier
        {
            l1 = m_L1;
        }
    }
    //+Z goes with the neutron beam direction.
    //Therefore the moderator is at a negative Z direction. X and Y are zero.
    source->setPos(0.0, 0.0, -1.0*l1);

    //Track detector IDs
    int det = 0;
    //--- Now we go through the list of monitors we got before. ----
    for(std::set<std::string>::const_iterator it=monitors.begin();it!=monitors.end();it++)
    {
        //Load the distance float.
        NXDouble z = entry.openNXDouble(*it + "/distance");
        z.load();
        //Monitor is just a simple sphere. Don't care about its real shape.
        Geometry::Detector *detector = new Geometry::Detector("mon",sphere,samplepos);
        //Assume that the monitor is in the horizontal plane, "distance" downstream.
        Geometry::V3D pos(0,0,*z());

        detector->setPos(pos);

        // set detector ID, add copy to instrument and mark it
        detector->setID(det);
        instrument->add(detector);
        instrument->markAsMonitor(detector);
        det++;
    }

    //--- Now we go through the list of detectors (banks) we got before. ----
    for(std::set<std::string,CompareBanks>::const_iterator it=banks.begin();it!=banks.end();it++)
    {
        // std::cout << "Opening bank " << *it << "\n";
        //The detector nxs entry.
        NXDetector nxDet = nxInstr.openNXDetector(*it);

        // Find the size of a pixel
        NXFloat distance = nxDet.openDistance();
        distance.load();
        NXFloat azimuth = nxDet.openAzimuthalAngle();
        azimuth.load();
        NXFloat polar = nxDet.openPolarAngle();
        polar.load();


        Geometry::V3D pos_corner;
        Geometry::V3D pos_x;
        Geometry::V3D pos_y;
        pos_corner.spherical_rad(distance(0,0), polar(0,0), azimuth(0,0));
        pos_x.spherical_rad(distance(0,1), polar(0,1), azimuth(0,1));
        pos_y.spherical_rad(distance(1,0), polar(1,0), azimuth(1,0));

        double radius = 0.5*((pos_corner-pos_x).norm() + (pos_corner-pos_y).norm());

        std::stringstream xmlPixelSphere;
        xmlPixelSphere << "<sphere id=\"some-shape\">" "<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" />"
                 "<radius val=\"" << radius << "\" />" "</sphere>";

        boost::shared_ptr<Geometry::Object> pixel_sphere = shapeCreator.createShape(xmlPixelSphere.str());

        /*
        NXFloat detSize = nxDet.openNXFloat("origin/shape/size");
        detSize.load();

        //Size of the detector
        double szX = detSize[0] != detSize[0] ? 0.001 : detSize[0]/2;
        double szY = detSize[1] != detSize[1] ? 0.001 : detSize[1]/2;
        //Fake 1mm thick detector.
        double szZ = 0.001; //detSize[2] != detSize[2] ? 0.001 : detSize[2]/2;


        std::ostringstream xmlShapeStream;
        xmlShapeStream
            << " <cuboid id=\"detector-shape\"> "
            << "<left-front-bottom-point x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
            << "<left-front-top-point  x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<szZ<<"\"  /> "
            << "<left-back-bottom-point  x=\""<<-szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
            << "<right-front-bottom-point  x=\""<<szX<<"\" y=\""<<szY<<"\" z=\""<<-szZ<<"\"  /> "
            << "</cuboid>";

        std::string xmlCuboidShape(xmlShapeStream.str());
        boost::shared_ptr<Geometry::Object> cuboidShape = shapeCreator.createShape(xmlCuboidShape);
        */

        //Now we calculate the bank orientation and shift
        Geometry::V3D shift;
        Geometry::Quat rot;
        this->getBankOrientation(nxDet,shift,rot);

        for(int i=0;i<distance.dim0();i++)
        for(int j=0;j<distance.dim1();j++)
        {
            float r = distance(i,j);
            float polar_angle = polar(i,j);
            float azimuth_angle = azimuth(i,j);

            // check for a NaN
            if (r != r || polar_angle != polar_angle || azimuth_angle != azimuth_angle)
            {
                r = 0.;
                polar_angle = 0.;
                azimuth_angle = 0.;
            }

            // Create a new detector. Instrument will take ownership of pointer so no need to delete.
            std::stringstream detname;
            detname << *it << ", (" << i << "," << j << ")";
            Geometry::Detector *detector = new Geometry::Detector(detname.str(), pixel_sphere, samplepos);
            Geometry::V3D pos;

            pos.spherical_rad(r, polar_angle, azimuth_angle);
            detector->setPos(pos);
            detector->setRot(rot);

            // set detector ID, add copy to instrument and mark it
            detector->setID(det);
            instrument->add(detector);
            instrument->markAsDetector(detector);
            det++;
        }
    }
}


/**  Get detector position and orientation from NXS file.
 *
 *   @param nxDet The NXDetector element
 *   @param shift The translation of the detector
 *   @param rot The orientation of the detector
 */
void LoadInstrumentFromSNSNexus::getBankOrientation(NXDetector nxDet, Geometry::V3D& shift, Geometry::Quat& rot)
{
    //Translation aka shift from the origin
    NXFloat translation = nxDet.openNXFloat("origin/translation/distance");
    translation.load();
    shift = Geometry::V3D(translation[0],translation[1],translation[2]);

    NXFloat orientation = nxDet.openNXFloat("origin/orientation/value");
    orientation.load();
    Geometry::V3D axis;

    /*
     - NXGeometry/orientation reference: The orientation information is stored as direction cosines.
     - It is a partial Direction Cosine Matrix or DCM?
     - As of May 2010.
     The direction cosines will be between the local coordinate directions and the reference directions
     (to origin or relative NXgeometry). Calling the local unit vectors (x',y',z') and the reference
     unit vectors (x,y,z) the six numbers will be
     [x' dot x, x' dot y, x' dot z, y' dot x, y' dot y, y' dot z]
     where "dot" is the scalar dot product (cosine of the angle between the unit vectors).
     The unit vectors in both the local and reference coordinates are right-handed and orthonormal.}
   */
    Geometry::V3D X(orientation[0],orientation[1],orientation[2]);
    Geometry::V3D Y(orientation[3],orientation[4],orientation[5]);
    Geometry::V3D Z = X.cross_prod(Y);

    //Crate the quaternion, using the constructor that uses a rotated reference frame.
    // See the Quat class for more info.
    rot = Geometry::Quat(X,Y,Z);
}

double LoadInstrumentFromSNSNexus::dblSqrt(double in)
{
    return sqrt(in);
}






/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadInstrumentFromSNSNexus::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the input workspace
  const Workspace_sptr localWorkspace = getProperty("Workspace");

  // Create the root Nexus class
  NXRoot root(getPropertyValue("Filename"));

  // Loop over entries loading each entry into a separate workspace
  for(std::vector<NXClassInfo>::const_iterator it=root.groups().begin();it!=root.groups().end();it++)
  if (it->nxclass == "NXentry")
  {
      this->loadInstrument(localWorkspace, root.openEntry(it->nxname));

      //Return after the first entry.
      return;
  }


  return;
}


} // namespace NeXus
} // namespace Mantid
