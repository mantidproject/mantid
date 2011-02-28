//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionEventReadDetCal.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/V3D.h"
#include <Poco/File.h>
#include <sstream>
#include <numeric>
#include <cmath>
#include <iomanip>

namespace Mantid
{
namespace Algorithms
{

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(DiffractionEventReadDetCal)

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;
  using namespace DataObjects;

    /// Constructor
    DiffractionEventReadDetCal::DiffractionEventReadDetCal() :
      API::Algorithm()
    {}

    /// Destructor
    DiffractionEventReadDetCal::~DiffractionEventReadDetCal()
    {}



/**
 * The intensity function calculates the intensity as a function of detector position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param detname :: The detector name
 * @param inname :: The workspace name
 */

  void DiffractionEventReadDetCal::center(double x, double y, double z, std::string detname, std::string inname)
  {

    MatrixWorkspace_sptr inputW = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve(inname));

    IAlgorithm_sptr alg1 = createSubAlgorithm("MoveInstrumentComponent");
    alg1->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    alg1->setPropertyValue("ComponentName", detname);
    alg1->setProperty("X", x);
    alg1->setProperty("Y", y);
    alg1->setProperty("Z", z);
    alg1->setPropertyValue("RelativePosition", "0");
    try
    {
      alg1->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run MoveInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing MoveInstrumentComponent as a sub algorithm.");
    }
} 

  /** Initialisation method
  */
  void DiffractionEventReadDetCal::init()
  {
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
                            "The workspace containing the geometry to be calibrated." );
  /*declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm." );*/

    declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".DetCal"), "The input filename of the ISAW DetCal file");

    return;
  }


  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void DiffractionEventReadDetCal::exec()
  {

    // Get the input workspace
    MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");

    //Get some stuff from the input workspace
    IInstrument_sptr inst = inputW->getInstrument();
    if (!inst)
      throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

    // set-up minimizer

    std::string inname = getProperty("InputWorkspace");
    std::string filename = getProperty("Filename");

    // Output summary to log file
    int idnum=0, count, id, nrows, ncols;
    double width, height, depth, detd, x, y, z, base_x, base_y, base_z, up_x, up_y, up_z;
    std::ifstream input(filename.c_str(), std::ios_base::in);
    std::string line;
    std::string detname;
    //Build a list of Rectangular Detectors
    std::vector<boost::shared_ptr<RectangularDetector> > detList;
    for (int i=0; i < inst->nelements(); i++)
    {
      boost::shared_ptr<RectangularDetector> det;
      boost::shared_ptr<ICompAssembly> assem;

      det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
     if (det)
        detList.push_back(det);
      else
      {
        //Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
        // We are not doing a full recursive search since that will be very long for lots of pixels.
        assem = boost::dynamic_pointer_cast<ICompAssembly>( (*inst)[i] );
        if (assem)
        {
          for (int j=0; j < assem->nelements(); j++)
          {
            det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem)[j] );
            if (det) detList.push_back(det);
          }
        }
      }
    }

    if (detList.size() == 0)
      throw std::runtime_error("This instrument does not have any RectangularDetector's. DiffractionEventReadDetCal cannot operate on this instrument at this time.");

    while(std::getline(input, line)) 
    {
      if(line[0] != '5') continue;

      std::stringstream(line) >> count >> id >> nrows >> ncols >> width >> height >> depth >> detd
                              >> x >> y >> z >> base_x >> base_y >> base_z >> up_x >> up_y >> up_z;
      // Convert from cm to m
      x = x * 0.01;
      y = y * 0.01;
      z = z * 0.01;
      boost::shared_ptr<RectangularDetector> det;
      det = detList[idnum];
      idnum++;
      if (det)
      {
        detname = det->getName();
        center(x, y, z, detname, inname);

        //These are the ISAW axes
        V3D rX = V3D(base_x, base_y, base_z);
        rX.normalize();
        V3D rY = V3D(up_x, up_y, up_z);
        rY.normalize();
        //V3D rZ=rX.cross_prod(rY);
        
        //These are the original axes
        V3D oX = V3D(1.,0.,0.);
        V3D oY = V3D(0.,1.,0.);
        V3D oZ = V3D(0.,0.,1.);

        //Axis that rotates X
        V3D ax1 = oX.cross_prod(rX);
        //Rotation angle from oX to rX
        double angle1 = oX.angle(rX);
        angle1 *=180.0/M_PI;
        //Create the first quaternion
        Quat Q1(angle1, ax1);

        //Now we rotate the original Y using Q1
        V3D roY = oY;
        Q1.rotate(roY);
        //Find the axis that rotates oYr onto rY
        V3D ax2 = roY.cross_prod(rY);
        double angle2 = roY.angle(rY);
        angle2 *=180.0/M_PI;
        Quat Q2(angle2, ax2);

        //Final = those two rotations in succession; Q1 is done first.
        Quat Rot = Q2 * Q1;

        // Then find the corresponding relative position
        boost::shared_ptr<IComponent> comp = inst->getComponentByName(detname);
        boost::shared_ptr<const IComponent> parent = comp->getParent();
        if (parent)
        {
            Quat rot0 = parent->getRelativeRot();
            rot0.inverse();
            Rot = Rot * rot0;
        }

        //Need to get the address to the base instrument component
        Geometry::ParameterMap& pmap = inputW->instrumentParameters();

        // Set or overwrite "rot" instrument parameter.
	pmap.addQuat(comp.get(),"rot",Rot);

      } 
    } 


    return;
  }


} // namespace Algorithm
} // namespace Mantid
