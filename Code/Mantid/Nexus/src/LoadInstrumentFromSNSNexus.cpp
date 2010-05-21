//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadInstrumentFromSNSNexus.h"
#include "MantidAPI/Instrument.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FileProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include <fstream>


namespace Mantid
{
namespace NeXus
{

DECLARE_ALGORITHM(LoadInstrumentFromSNSNexus)

using namespace Kernel;
using namespace API;

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

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, std::vector<std::string>(1, "nxs")),
		  "The name (including its full or relative path) of the Nexus file to\n"
		  "attempt to load the instrument from. The file extension must either be\n"
		  ".nxs or .NXS" );
}






/** Loads the instrument from the SNS file
 *  @param localWS The workspace
 *  @param entry The entry in the Nexus file
 *  @param banks A sorted set of bank names
 *  @param monitors A set of monitor names
 */
void LoadInstrumentFromSNSNexus::loadInstrument(API::Workspace_sptr localWS,
                            NXEntry entry)

{

    std::cout << "LoadInstrumentFromSNSNexus 00\n";
    std::set<std::string,CompareBanks> banks;
    std::set<std::string> monitors;

    for(std::vector<NXClassInfo>::const_iterator it=entry.groups().begin();it!=entry.groups().end();it++)
    if (it->nxclass == "NXdata") // Count detectors
    {
      std::cout << "Found bank " << it->nxname << "\n";
        banks.insert(it->nxname); // sort the bank names
    }
    else if (it->nxclass == "NXmonitor") // Count monitors
    {
        monitors.insert(it->nxname);
    }

    DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(localWS);

    // Clear off any existing instrument for this workspace
    ws->setInstrument(boost::shared_ptr<Instrument>(new Instrument));

    // Get reference to Instrument and set its name
    boost::shared_ptr<API::Instrument> instrument = ws->getBaseInstrument();

    std::string xmlSphere = "<sphere id=\"some-shape\">"
             "<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" />"
             "<radius val=\"0.03\" />"
             "</sphere>";

    Geometry::ShapeFactory shapeCreator;
    boost::shared_ptr<Geometry::Object> sphere = shapeCreator.createShape(xmlSphere);

    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample",sphere,instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    samplepos->setPos(0.0,0.0,0.0);

    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",sphere,instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);

    NXInstrument nxInstr = entry.openNXInstrument("instrument");
    NXChar name = nxInstr.openNXChar("name");
    name.load();
    instrument->setName(name());

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
    source->setPos(0.0,0.0,-1.0*l1);

    int det = 1;
    for(std::set<std::string>::const_iterator it=monitors.begin();it!=monitors.end();it++)
    {

        NXDouble z = entry.openNXDouble(*it + "/distance");
        z.load();
        Geometry::Detector *detector = new Geometry::Detector("mon",sphere,samplepos);
        Geometry::V3D pos(0,0,*z());

        detector->setPos(pos);

        // set detector ID, add copy to instrument and mark it
        detector->setID(det);
        instrument->add(detector);
        instrument->markAsMonitor(detector);
        det++;
    }

    for(std::set<std::string,CompareBanks>::const_iterator it=banks.begin();it!=banks.end();it++)
    {
        std::cout << "Opening bank " << *it << "\n";

        NXDetector nxDet = nxInstr.openNXDetector(*it);

        NXFloat detSize = nxDet.openNXFloat("origin/shape/size");
        detSize.load();

        double szX = detSize[0] != detSize[0] ? 0.001 : detSize[0]/2;
        double szY = detSize[1] != detSize[1] ? 0.001 : detSize[1]/2;
        double szZ = 0.001; //detSize[2] != detSize[2] ? 0.001 : detSize[2]/2;

        std::ostringstream xmlShapeStream;
        xmlShapeStream
            << " <cuboid id=\"detector-shape\"> "
            << "<left-front-bottom-point x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
            << "<left-front-top-point  x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<szZ<<"\"  /> "
            << "<left-back-bottom-point  x=\""<<-szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
            << "<right-front-bottom-point  x=\""<<szX<<"\" y=\""<<szY<<"\" z=\""<<-szZ<<"\"  /> "
            << "</cuboid>";

        std::string xmlShape(xmlShapeStream.str());
        boost::shared_ptr<Geometry::Object> shape = shapeCreator.createShape(xmlShape);

        Geometry::V3D shift;
        Geometry::Quat rot;
        this->getBankOrientation(nxDet,shift,rot);

        NXFloat distance = nxDet.openDistance();
        distance.load();

        std::cout << "distance.dim0()" << distance.dim0() << "\n";
        std::cout << "distance.dim1()" << distance.dim1() << "\n";

        NXFloat azimuth = nxDet.openAzimuthalAngle();
        azimuth.load();
        NXFloat polar = nxDet.openPolarAngle();
        polar.load();
        for(int i=0;i<distance.dim0();i++)
        for(int j=0;j<distance.dim1();j++)
        {
            float r = distance(i,j);
            float angle = polar(i,j);
            float phi = azimuth(i,j);

            // check for a NaN
            if (r != r || angle != angle || phi != phi)
            {
                r = 0.;
                angle = 0.;
                phi = 0.;
            }

            // Create a new detector. Instrument will take ownership of pointer so no need to delete.
            Geometry::Detector *detector = new Geometry::Detector("det",shape,samplepos);
            Geometry::V3D pos;

            pos.spherical(r, angle/M_PI*180., phi/M_PI*180.);
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

/** Calculate rotation axis from direction cosines.
 */
//void LoadSNSNexus::calcRotation(double xx,double xy,double xz,double yx,double yy,double yz,double& angle, Geometry::V3D& axis)
void LoadInstrumentFromSNSNexus::calcRotation(const Geometry::V3D& X,const Geometry::V3D& Y,const Geometry::V3D& Z,double& angle, Geometry::V3D& axis)
{
    //double angle1 = 10.;
    //Geometry::V3D axis1(1,2,0);
    //axis1.normalize();
    //std::cerr<<"axis1 "<<axis1<<' '<<angle1<<'\n';
    //Geometry::Quat rot(angle1,axis1);
    //Geometry::V3D X1(1,0,0);
    //Geometry::V3D Y1(0,1,0);
    //Geometry::V3D Z1(0,0,1);
    //rot.rotate(X1);
    //rot.rotate(Y1);
    //rot.rotate(Z1);

    double xx = X.X();
    double xy = X.Y();
    double xz = X.Z();

    //double yx = Y.X();
    double yy = Y.Y();
    double yz = Y.Z();

    double zx = Z.X();
    double zy = Z.Y();
    double zz = Z.Z();

    //std::cerr<<xx<<' '<<xy<<' '<<xz<<'\n';
    //std::cerr<<yx<<' '<<yy<<' '<<yz<<'\n';
    //std::cerr<<zx<<' '<<zy<<' '<<zz<<'\n';

    if (xx == 1.)
    {
        axis(1,0,0);
        angle = acos(yy)/M_PI*180.;
        if (yz < 0) angle = -angle;
    }
    else if (yy == 1.)
    {
        axis(0,1,0);
        angle = acos(zz)/M_PI*180.;
        if (zx < 0) angle = -angle;
    }
    else if (zz == 1.)
    {
        axis(0,0,1);
        angle = acos(xx)/M_PI*180.;
        if (xy < 0) angle = -angle;
    }
    else
    {
        std::cout << "The case of arbitrary direction cosines is not implemented yet7\n";
        throw std::runtime_error("The case of arbitrary direction cosines is not implemented yet");

        double A1 = xx - 1. + xz*zx/(1.-zz);
        double A2 = xy + xz*zy/(1.-zz);
        double A = - A2 / A1;

        double B = sqrt( (1. - xx)/(1. - yy) );

        std::cerr<<"A,B="<<A<<' '<<B<<'\n';

        double cosY = sqrt( (1. - B*B)/(A*A - B*B) );
        double cosX = A*cosY;
        double cosZ = sqrt( 1. - cosX*cosX - cosY*cosY );

        Geometry::V3D N(cosX,cosY,cosZ);

        double tstX = N.scalar_prod(X);
        double tstY = N.scalar_prod(Y);
        double tstZ = N.scalar_prod(Z);

        std::cerr<<"Test:\n";
        std::cerr<<cosX<<' '<<cosY<<' '<<cosZ<<'\n';
        std::cerr<<tstX<<' '<<tstY<<' '<<tstZ<<'\n';

        axis(cosX,cosY,cosZ);

    }


}

/**  Get detector position and orientation
 *   @param nxDet The NXDetector element
 *   @param shift The translation of the detector
 *   @param rot The orientation of the detector
 */
void LoadInstrumentFromSNSNexus::getBankOrientation(NXDetector nxDet, Geometry::V3D& shift, Geometry::Quat& rot)
{
    NXFloat translation = nxDet.openNXFloat("origin/translation/distance");
    translation.load();
    shift = Geometry::V3D(translation[0],translation[1],translation[2]);

    NXFloat orientation = nxDet.openNXFloat("origin/orientation/value");
    orientation.load();
    double angle;
    Geometry::V3D axis;

    Geometry::V3D X(orientation[0],orientation[1],orientation[2]);
    Geometry::V3D Y(orientation[3],orientation[4],orientation[5]);
    Geometry::V3D Z = X.cross_prod(Y);

    this->calcRotation(X,Y,Z,angle,axis);

    std::cerr<<"rot axis "<<axis<<'\n';
    std::cerr<<"angle "<<angle<<'\n';

    rot = Geometry::Quat(angle,axis);
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
