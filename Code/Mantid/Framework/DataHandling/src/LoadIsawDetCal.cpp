/*WIKI* 




Moves the detectors in an instrument using the origin and 2 vectors of the rotated plane from an ISAW DetCal file.



==Usage==
'''Python'''
    LoadIsawDetCal("SNAP_4111","SNAP.DetCal")

'''C++'''
    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("LoadIsawDetCal");
    alg->setPropertyValue("InputWorkspace", "SNAP_4111");
    alg->setPropertyValue("Filename", "SNAP.DetCal");
    alg->execute();





*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadIsawDetCal.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/V3D.h"
#include <Poco/File.h>
#include <sstream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace DataHandling
{

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(LoadIsawDetCal)
  
  /// Sets documentation strings for this algorithm
  void LoadIsawDetCal::initDocs()
  {
    this->setWikiSummary("Since ISAW already has the capability to calibrate the instrument using single crystal peaks, this algorithm leverages this in mantid. It loads in a detcal file from ISAW and moves all of the detector panels accordingly. The target instruments for this feature are SNAP and TOPAZ. ");
    this->setOptionalMessage("Since ISAW already has the capability to calibrate the instrument using single crystal peaks, this algorithm leverages this in mantid. It loads in a detcal file from ISAW and moves all of the detector panels accordingly. The target instruments for this feature are SNAP and TOPAZ.");
  }
  

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;
  using namespace DataObjects;

    /// Constructor
    LoadIsawDetCal::LoadIsawDetCal() :
      API::Algorithm()
    {}

    /// Destructor
    LoadIsawDetCal::~LoadIsawDetCal()
    {}



/**
 * The intensity function calculates the intensity as a function of detector position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param detname :: The detector name
 * @param inname :: The workspace name
 */

  void LoadIsawDetCal::center(double x, double y, double z, std::string detname, std::string inname)
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
    alg1->executeAsSubAlg();
} 

  /** Initialisation method
  */
  void LoadIsawDetCal::init()
  {
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,new InstrumentValidator<>),
                            "The workspace containing the geometry to be calibrated." );
  /*declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm." );*/

    declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".DetCal"), "The input filename of the ISAW DetCal file (East banks for SNAP) ");

    declareProperty(new API::FileProperty("Filename2", "", API::FileProperty::OptionalLoad, ".DetCal"), "The input filename of the second ISAW DetCal file (West banks for SNAP) ");

    return;
  }


  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void LoadIsawDetCal::exec()
  {

    // Get the input workspace
    MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");

    //Get some stuff from the input workspace
    Instrument_const_sptr inst = inputW->getInstrument();
    std::string instname = inst->getName();
    Geometry::Instrument_sptr instrument(new Geometry::Instrument(instname));
    inputW->setInstrument(instrument);

    // set-up minimizer

    std::string inname = getProperty("InputWorkspace");
    std::string filename = getProperty("Filename");
    std::string filename2 = getProperty("Filename2");

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
      boost::shared_ptr<ICompAssembly> assem2;
  
      det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
      if (det)
      {
        detList.push_back(det);
      }
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
            if (det)
            {
              detList.push_back(det);
  
            }
            else
            {
              //Also, look in the second sub-level for RectangularDetectors (e.g. PG3).
              // We are not doing a full recursive search since that will be very long for lots of pixels.
              assem2 = boost::dynamic_pointer_cast<ICompAssembly>( (*assem)[j] );
              if (assem2)
              {
                for (int k=0; k < assem2->nelements(); k++)
                {
                  det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem2)[k] );
                  if (det)
                  {
                    detList.push_back(det);
                  }
                }
              }
            }
          }
        }
      }
    }


    if (detList.size() == 0)
      throw std::runtime_error("This instrument does not have any RectangularDetector's. LoadIsawDetCal cannot operate on this instrument at this time.");

    while(std::getline(input, line)) 
    {
      if(line[0] == '7')
      {
        double mL1, mT0;
        std::stringstream(line) >> count >> mL1 >> mT0;
        //mT0 and time of flight are both in microsec
        IAlgorithm_sptr alg1 = createSubAlgorithm("ChangeBinOffset");
        alg1->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
        alg1->setProperty("Offset", mT0);
        alg1->executeAsSubAlg();
        Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("sample-position", instrument.get());
        instrument->add(samplepos);
        instrument->markAsSamplePos(samplepos);
        samplepos->setPos(0.0, 0.0, 0.0);
    
        Geometry::ObjComponent *source = new Geometry::ObjComponent("moderator", instrument.get());
        instrument->add(source);
        instrument->markAsSource(source);
        // Convert from cm to m
        source->setPos(0.0, 0.0, -0.01 * mL1);

      }

      if(line[0] != '5') continue;

      std::stringstream(line) >> count >> id >> nrows >> ncols >> width >> height >> depth >> detd
                              >> x >> y >> z >> base_x >> base_y >> base_z >> up_x >> up_y >> up_z;
      if( id == 10 && instname == "SNAP" && filename2 != "")
      {
        input.close();
        input.open(filename2.c_str());
        while(std::getline(input, line))
        {
          if(line[0] != '5') continue;

          std::stringstream(line) >> count >> id >> nrows >> ncols >> width >> height >> depth >> detd
                                  >> x >> y >> z >> base_x >> base_y >> base_z >> up_x >> up_y >> up_z;
          if (id==10)break;
        }
      }
      boost::shared_ptr<RectangularDetector> det;
      std::ostringstream Detbank;
      Detbank <<"bank"<<id;
      // Loop through detectors to match names with number from DetCal file
      for (int i=0; i < static_cast<int>(detList.size()); i++)
        if(detList[i]->getName().compare(Detbank.str()) == 0) idnum=i;
      det = detList[idnum];
      if (det)
      {
        // Convert from cm to m
        width *= 0.01;
        height *= 0.01;
        double xstep = width / det->xpixels();
        double ystep = height / det->ypixels();
        double xstart = -width * 0.5;
        double ystart = -height * 0.5;
        Geometry::RectangularDetector * bank = new Geometry::RectangularDetector(det->getName(), instrument.get());
        std::string pixel_width_str = "0.000309";
        std::string pixel_height_str = "0.000309";
        std::string pixel_depth_str = "-0.0001";
        std::string detXML =   "<cuboid id=\"pixel\">"
          "<left-front-bottom-point   x= \""+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\"0\"  />"
          "<left-front-top-point      x= \""+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\""+pixel_depth_str+"\"  />"
          "<left-back-bottom-point    x=\"-"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\"0\"  />"
          "<right-front-bottom-point  x= \""+pixel_width_str+"\" y= \""+pixel_height_str+"\" z=\"0\"  />"
          "</cuboid>";
        Geometry::Object_sptr shape = Geometry::ShapeFactory().createShape(detXML);
        bank->initialize(shape, det->xpixels(), xstart, xstep, det->ypixels(), ystart, ystep, det->idstart(), det->idfillbyfirst_y(), det->idstepbyrow(), det->idstep());
      try
      {
        for (int x=0; x < bank->nelements(); x++)
        {
          boost::shared_ptr<Geometry::ICompAssembly> xColumn = boost::dynamic_pointer_cast<Geometry::ICompAssembly>((*bank)[x]);
          for (int y=0; y < xColumn->nelements(); y++)
          {
            boost::shared_ptr<Geometry::Detector> detector = boost::dynamic_pointer_cast<Geometry::Detector>((*xColumn)[y]);
            if (detector)
            {
               //Mark it as a detector (add to the instrument cache)
               instrument->markAsDetector(detector.get());
            }
          }
        }
      }
      catch(Kernel::Exception::ExistsError&)
      {
         throw Kernel::Exception::InstrumentDefinitionError(
           "Duplicate detector ID found when adding RectangularDetector " + det->getName() + " in XML instrument file");
      }
              
        // Convert from cm to m
        x *= 0.01;
        y *= 0.01;
        z *= 0.01;
        detname = det->getName();
        bank->setPos(x,y,z);

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
        boost::shared_ptr<const IComponent> comp = instrument->getComponentByName(detname);
        boost::shared_ptr<const IComponent> parent = comp->getParent();
        if (parent)
        {
            Quat rot0 = parent->getRelativeRot();
            rot0.inverse();
            Rot = Rot * rot0;
        }
        boost::shared_ptr<const IComponent>grandparent = parent->getParent();
        if (grandparent)
        {
            Quat rot0 = grandparent->getRelativeRot();
            rot0.inverse();
            Rot = Rot * rot0;
        }

        // Set or overwrite "rot" instrument parameter.
        bank->setRot(Rot);

      } 
    } 


    return;
  }


} // namespace Algorithm
} // namespace Mantid
