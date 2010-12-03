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
 * @param x The shift along the X-axis
 * @param y The shift along the Y-axis
 * @param z The shift along the Z-axis
 * @param ax The rotation vector for the X-axis
 * @param ay The rotation vector for the Y-axis
 * @param az The rotation vector for the Z-axis
 * @param angle The rotation around the Y-axis
 * @param detname The detector name
 * @param inname The workspace name
 */

  void DiffractionEventReadDetCal::intensity(double x, double y, double z, double ax, double ay, double az, double angle, std::string detname, std::string inname)
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

    IAlgorithm_sptr algx = createSubAlgorithm("RotateInstrumentComponent");
    algx->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    algx->setPropertyValue("ComponentName", detname);
    algx->setProperty("X", ax);
    algx->setProperty("Y", ay);
    algx->setProperty("Z", az);
    algx->setProperty("Angle", angle);
    algx->setPropertyValue("RelativeRotation", "0");
    try
    {
      algx->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run RotateInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing RotateInstrumentComponent as a sub algorithm.");
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
    MatrixWorkspace_const_sptr matrixInWS = getProperty("InputWorkspace");
    EventWorkspace_const_sptr inputW = boost::dynamic_pointer_cast<const EventWorkspace>( matrixInWS );
    if (!inputW)
      throw std::invalid_argument("InputWorkspace should be an EventWorkspace.");

    //Get some stuff from the input workspace
    IInstrument_sptr inst = inputW->getInstrument();
    if (!inst)
      throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

    // set-up minimizer

    std::string inname = getProperty("InputWorkspace");
    std::string filename = getProperty("Filename");

    // Output summary to log file
    int count, id, nrows, ncols;
    double width, height, depth, detd, x, y, z, base_x, base_y, base_z, up_x, up_y, up_z;
    double ax, ay, az, roty=0;
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
      throw std::runtime_error("This instrument does not have any RectangularDetector's. SumNeighbors cannot operate on this instrument at this time.");

    int i=0;
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
      det = detList[i];
      i++;
      if (det)
      {
        detname = det->getName();
        V3D base = V3D(base_x, base_y, base_z);
        base.normalize();
        V3D up = V3D(up_x, up_y, up_z);
        up.normalize();
        Quat cr3 = Quat(base, up);
        cr3.getAngleAxis(roty, ax, ay, az);
        //Added 180 just like IDF
        roty += 180.0;
        V3D out  = V3D(ax, ay, az);
        V3D ra3 = out.cross_prod(V3D(0,0,1));
        ax = ra3.X();
        ay = ra3.Y();
        az = ra3.Z();
        intensity(x, y, z, ax, ay, az, roty, detname, inname);
      } 
    } 


    return;
  }


} // namespace Algorithm
} // namespace Mantid
