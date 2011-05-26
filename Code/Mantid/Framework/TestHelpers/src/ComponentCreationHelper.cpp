//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadInstrumentHelper.h"

#include <Poco/Path.h>
#include <boost/shared_array.hpp>
#include "MantidGeometry/IDetector.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::detid_t;
using Mantid::specid_t;

namespace ComponentCreationHelper
{
  //----------------------------------------------------------------------------------------------
  /**
   * Create a capped cylinder object
   */
  Object_sptr createCappedCylinder(double radius, double height, const V3D & baseCentre, const V3D & axis, const std::string & id)
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
  

  //----------------------------------------------------------------------------------------------

  /**
   * Return the XML for a sphere.
   */
  std::string sphereXML(double radius, const V3D & centre, const std::string & id)
  {
    std::ostringstream xml;
    xml << "<sphere id=\"" << id <<  "\">"
    << "<centre x=\"" << centre.X() << "\"  y=\"" << centre.Y() << "\" z=\"" << centre.Z() << "\" />"
    << "<radius val=\"" << radius << "\" />"
    << "</sphere>";
    return xml.str();
  }

  /**
   * Create a sphere object
   */
  Object_sptr createSphere(double radius, const V3D & centre, const std::string & id)
  {
    ShapeFactory shapeMaker;
    return shapeMaker.createShape(sphereXML(radius, centre, id));
  }


  //----------------------------------------------------------------------------------------------
  /** Create a cuboid shape for your pixels */
  Object_sptr createCuboid(double x_side_length, double y_side_length, double z_side_length)
  {
    double szX = x_side_length;
    double szY = (y_side_length == -1.0 ? szX : y_side_length);
    double szZ = (z_side_length == -1.0 ? szX : z_side_length);
    std::ostringstream xmlShapeStream;
    xmlShapeStream
    << " <cuboid id=\"detector-shape\"> "
    << "<left-front-bottom-point x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
    << "<left-front-top-point  x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<szZ<<"\"  /> "
    << "<left-back-bottom-point  x=\""<<-szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> "
    << "<right-front-bottom-point  x=\""<<szX<<"\" y=\""<<szY<<"\" z=\""<<-szZ<<"\"  /> "
    << "</cuboid>";

    std::string xmlCuboidShape(xmlShapeStream.str());
    ShapeFactory shapeCreator;
    Object_sptr cuboidShape = shapeCreator.createShape(xmlCuboidShape);
    return cuboidShape;
  }


  //----------------------------------------------------------------------------------------------
  /**
  * Create a component assembly at the origin made up of 4 cylindrical detectors
  */
  boost::shared_ptr<CompAssembly> createTestAssemblyOfFourCylinders()
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
   * Create an object component that has a defined shape
   */
  ObjComponent * createSingleObjectComponent()
  {
    Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(0.5, 1.5, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube");
    return new ObjComponent("pixel", pixelShape);
  }

  /**
   * Create a hollow shell, i.e. the intersection of two spheres or radius r1 and r2
   */
  Object_sptr createHollowShell(double innerRadius, double outerRadius, const V3D & centre)
  {
    std::string wholeXML = 
      sphereXML(innerRadius, centre, "inner") + "\n" + 
      sphereXML(outerRadius, centre, "outer") + "\n" + 
      "<algebra val=\"(outer (# inner))\" />";
    
    ShapeFactory shapeMaker;
    return shapeMaker.createShape(wholeXML);
  }

  //----------------------------------------------------------------------------------------------
  /**
   * Create a detector group containing 5 detectors
   */
  boost::shared_ptr<DetectorGroup> createDetectorGroupWith5CylindricalDetectors()
  {
    const int ndets = 5;
    std::vector<boost::shared_ptr<IDetector> > groupMembers(ndets);
    // One object
    Object_sptr detShape = ComponentCreationHelper::createCappedCylinder(0.5, 1.5, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube"); 
    for( int i = 0; i < ndets; ++i )
    {
      std::ostringstream os;
      os << "d" << i;
      boost::shared_ptr<Detector> det(new Detector(os.str(), i+1, detShape, NULL));
      det->setPos((double)(i+1), 2.0, 2.0);
      groupMembers[i] = det;
    }

    return boost::shared_ptr<DetectorGroup>(new DetectorGroup(groupMembers, false));
  }

   //----------------------------------------------------------------------------------------------
  /**
   * Create a group of detectors arranged in a ring;
   */
  boost::shared_ptr<DetectorGroup> createRingOfCylindricalDetectors(const double R_min, const double R_max, const double z0)
  {

    std::vector<boost::shared_ptr<IDetector> > groupMembers;
    // One object
    double R0=0.5;
    double h =1.5;
    Object_sptr detShape = ComponentCreationHelper::createCappedCylinder(R0, h, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube"); 

    int NY=int(ceil(2*R_max/h)+1);
    int NX=int(ceil(2*R_max/R0)+1);
    double y_bl = NY*h;
    double x_bl = NX*R0;

  
    double Rmin2(R_min*R_min),Rmax2(R_max*R_max);

    int ic(0);
    for(int j=0;j<NY;j++){
        double y=-0.5*y_bl+j*h;
        for(int i=0;i<NX;i++){
            double x = -0.5*x_bl+i*R0;
            double Rsq = x*x+y*y;
            if(Rsq>=Rmin2 && Rsq<Rmax2){
                  std::ostringstream os;
                  os << "d" << ic;
                 boost::shared_ptr<Detector> det(new Detector(os.str(), ic+1, detShape, NULL));
                 det->setPos(x, y, z0);
                 groupMembers.push_back(det);
            }

          ic++;
        }
    }
    return boost::shared_ptr<DetectorGroup>(new DetectorGroup(groupMembers, false));
  }

  //----------------------------------------------------------------------------------------------
  /**
   * Create a group of two monitors
   */
  boost::shared_ptr<DetectorGroup> createGroupOfTwoMonitors()
  {
    const int ndets(2);
    std::vector<boost::shared_ptr<IDetector> > groupMembers(ndets);
    for( int i = 0; i < ndets; ++i )
    {
      std::ostringstream os;
      os << "m" << i;
      boost::shared_ptr<Detector> det(new Detector(os.str(), i+1, NULL));
      det->setPos((double)(i+1), 2.0, 2.0);
      det->markAsMonitor();
      groupMembers[i] = det;
    }
    return boost::shared_ptr<DetectorGroup>(new DetectorGroup(groupMembers, false));
  }


  //----------------------------------------------------------------------------------------------
  /**
   * Create an test instrument with n panels of 9 cylindrical detectors, a source and spherical sample shape.
   * Detector ID's start at 1.
   *
   * @param num_banks: number of 9-cylinder banks to create
   * @param verbose: prints out the instrument after creation.
   */
  IInstrument_sptr createTestInstrumentCylindrical(int num_banks, bool verbose)
  {
    boost::shared_ptr<Instrument> testInst(new Instrument("basic"));

    const double cylRadius(0.004);
    const double cylHeight(0.0002);
    // One object
    Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(cylRadius, cylHeight, V3D(0.0,-cylHeight/2.0,0.0), V3D(0.,1.0,0.), "pixel-shape");

    //Just increment pixel ID's
    int pixelID = 1;

    for (int banknum=1; banknum <= num_banks; banknum++)
    {
      //Make a new bank
      std::ostringstream bankname;
      bankname << "bank" << banknum;
      CompAssembly *bank = new CompAssembly(bankname.str());

      // Four object components
      for( int i = -1; i < 2; ++i )
      {
        for( int j = -1; j < 2; ++j )
        {
          std::ostringstream lexer;
          lexer << "pixel-(" << j << "," << i << ")";
          Detector * physicalPixel = new Detector(lexer.str(), pixelID, pixelShape, bank);
          const double xpos = j*cylRadius*2.0;
          const double ypos = i*cylHeight;
          physicalPixel->setPos(xpos, ypos,0.0);
          pixelID++;
          bank->add(physicalPixel);
          testInst->markAsDetector(physicalPixel);
        }
      }

      testInst->add(bank);
      bank->setPos(V3D(0.0, 0.0, 5.0*banknum));
    }

    //Define a source component
    ObjComponent *source = new ObjComponent("moderator", Object_sptr(new Object), testInst.get());
    source->setPos(V3D(0.0, 0.0, -10.));
    testInst->add(source);
    testInst->markAsSource(source);

    // Define a sample as a simple sphere
    Object_sptr sampleSphere = createSphere(0.001, V3D(0.0, 0.0, 0.0), "sample-shape");
    ObjComponent *sample = new ObjComponent("sample", sampleSphere, testInst.get());
    testInst->setPos(0.0, 0.0, 0.0);
    testInst->add(sample);
    testInst->markAsSamplePos(sample);

    if( verbose )
    {
      std::cout << "\n\n=== Testing bank positions ==\n";
      const int nchilds = testInst->nelements();
      for(int i = 0; i < nchilds; ++i )
      {
        boost::shared_ptr<IComponent> child = testInst->getChild(i);
        std::cout << "Component " << i << " at pos " << child->getPos() << "\n";
        if( boost::shared_ptr<ICompAssembly> assem = boost::dynamic_pointer_cast<ICompAssembly>(child) )
        {
          for(int j = 0; j < assem->nelements(); ++j )
          {
            boost::shared_ptr<IComponent> comp = assem->getChild(j);
            std::cout << "Child " << j << " at pos " << comp->getPos() << "\n";
          }
        }
      }
      std::cout << "==================================\n";
    }
    
    return testInst;
  }

  //----------------------------------------------------------------------------------------------
  /**
   * Create an test instrument with n panels of rectangular detectors, pixels*pixels in size,
   * a source and spherical sample shape.
   *
   * Banks' lower-left corner is at position (0,0,5*banknum) and they go up to (pixels*0.008, pixels*0.008, Z)
   * Pixels are 4 mm wide.
   *
   * @param num_banks: number of rectangular banks to create
   * @param pixels :: number of pixels in each direction.
   */
  IInstrument_sptr createTestInstrumentRectangular(int num_banks, int pixels, double pixelSpacing)
  {
    boost::shared_ptr<Instrument> testInst(new Instrument("basic_rect"));

    const double cylRadius(pixelSpacing/2);
    const double cylHeight(0.0002);
    // One object
    Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(cylRadius, cylHeight, V3D(0.0,-cylHeight/2.0,0.0), V3D(0.,1.0,0.), "pixel-shape");

    for (int banknum=1; banknum <= num_banks; banknum++)
    {
      //Make a new bank
      std::ostringstream bankname;
      bankname << "bank" << banknum;

      RectangularDetector * bank = new RectangularDetector(bankname.str());
      bank->initialize(pixelShape,
          pixels, 0.0, pixelSpacing,
          pixels, 0.0, pixelSpacing,
          banknum*pixels*pixels, true, pixels);

      // Mark them all as detectors
      for (int x=0; x<pixels; x++)
        for (int y=0; y<pixels; y++)
        {
          boost::shared_ptr<Detector> detector = bank->getAtXY(x,y);
          if (detector)
            //Mark it as a detector (add to the instrument cache)
            testInst->markAsDetector(detector.get());
        }

      testInst->add(bank);
      bank->setPos(V3D(0.0, 0.0, 5.0*banknum));
    }

    //Define a source component
    ObjComponent *source = new ObjComponent("moderator", Object_sptr(new Object), testInst.get());
    source->setPos(V3D(0.0, 0.0, -10.));
    testInst->add(source);
    testInst->markAsSource(source);

    // Define a sample as a simple sphere
    Object_sptr sampleSphere = createSphere(0.001, V3D(0.0, 0.0, 0.0), "sample-shape");
    ObjComponent *sample = new ObjComponent("sample", sampleSphere, testInst.get());
    testInst->setPos(0.0, 0.0, 0.0);
    testInst->add(sample);
    testInst->markAsSamplePos(sample);

    return testInst;
  }
}

/*****************************************************
 * SANS instrument helper class
 *****************************************************/

// Number of detector pixels in each dimension
const int SANSInstrumentCreationHelper::nBins = 30;
// The test instrument has 2 monitors
const int SANSInstrumentCreationHelper::nMonitors = 2;

  /*
   * Generate a SANS test workspace, with instrument geometry.
   * The geometry is the SANSTEST geometry, with a 30x30 pixel 2D detector.
   *
   * @param workspace: name of the workspace to be created.
   */
Workspace2D_sptr SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(std::string workspace)
{
  // Create a test workspace with test data with a well defined peak
  // The test instrument has two monitor channels
  Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace123(nBins*nBins+nMonitors,1,1);
  AnalysisDataService::Instance().addOrReplace(workspace, ws);
  ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
  ws->setYUnit("");
  for (std::size_t i = 0; i < ws->getNumberHistograms(); ++i)
  {
    ws->getAxis(1)->spectraNo(i) = static_cast<int>(i);
  }
  
  // Load instrument geometry
  runLoadInstrument("SANSTEST", ws);
  runLoadMappingTable(ws, nBins, nBins);
  
  return ws;
}

  /** Run the sub-algorithm LoadInstrument (as for LoadRaw)
   * @param inst_name :: The name written in the Nexus file
   * @param workspace :: The workspace to insert the instrument into
   */
  void SANSInstrumentCreationHelper::runLoadInstrument(const std::string & inst_name,
      Workspace2D_sptr workspace)
  {
    // Determine the search directory for XML instrument definition files (IDFs)
    std::string directoryName = Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory();

    // For Nexus Mantid processed, Instrument XML file name is read from nexus
    std::string instrumentID = inst_name;
    // force ID to upper case
    std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
    std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

//    Mantid::DataHandling::LoadInstrumentHelper helper;
//    std::string fullPathIDF = helper.getInstrumentFilename(instrumentID, Mantid::Kernel::DateAndTime::get_current_time().to_ISO8601_string());
    
    Mantid::DataHandling::LoadInstrument loadInst;
    loadInst.initialize();
    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    loadInst.setPropertyValue("Filename", fullPathIDF);
    loadInst.setProperty<MatrixWorkspace_sptr> ("Workspace", workspace);
    loadInst.execute();

  }

  /**
   * Populate spectra mapping to detector IDs
   *
   * @param workspace: Workspace2D object
   * @param nxbins: number of bins in X
   * @param nybins: number of bins in Y
   */
  void SANSInstrumentCreationHelper::runLoadMappingTable(Workspace2D_sptr workspace, int nxbins, int nybins)
  {
    // Get the number of monitor channels
    size_t nMonitors(0);
    size_t nXbins,nYbins;
    boost::shared_ptr<Instrument> instrument = workspace->getBaseInstrument();
    std::vector<detid_t> monitors = instrument->getMonitors();
    nMonitors = monitors.size();

    // Number of monitors should be consistent with data file format
    if( nMonitors != 2 ) {
      std::stringstream error;
      error << "Geometry error for " << instrument->getName() <<
          ": Spice data format defines 2 monitors, " << nMonitors << " were/was found";
      throw std::runtime_error(error.str());
    }
    if(nxbins>=0){
        nXbins=size_t(nxbins);
    }else{
        throw std::invalid_argument("number of x-bins < 0");
    }
    if(nybins>=0){
        nYbins=size_t(nybins);
    }else{
        throw std::invalid_argument("number of y-bins < 0");
    }


    size_t ndet = nxbins*nybins + nMonitors;
    boost::shared_array<detid_t> udet(new detid_t[ndet]);
    boost::shared_array<specid_t> spec(new specid_t[ndet]);

    // Generate mapping of detector/channel IDs to spectrum ID

    // Detector/channel counter
    size_t icount = 0;

    // Monitor: IDs start at 1 and increment by 1
    for(size_t i=0; i<nMonitors; i++)
    {
      spec[icount] = specid_t(icount);
      udet[icount] = detid_t(icount+1);
      icount++;
    }

    // Detector pixels
    for(size_t ix=0; ix<nXbins; ix++)
    {
      for(size_t iy=0; iy<nYbins; iy++)
      {
        spec[icount] = specid_t(icount);
        udet[icount] = detid_t(1000000 + iy*1000 + ix);
        icount++;
      }
    }

    // Populate the Spectra Map with parameters
    workspace->replaceSpectraMap(new SpectraDetectorMap(spec.get(), udet.get(), ndet));
  }
