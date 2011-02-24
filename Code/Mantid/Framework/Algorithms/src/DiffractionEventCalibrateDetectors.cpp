//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionEventCalibrateDetectors.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAlgorithms/GSLFunctions.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FileProperty.h"
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
  DECLARE_ALGORITHM(DiffractionEventCalibrateDetectors)

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;
  using namespace DataObjects;

    /// Constructor
    DiffractionEventCalibrateDetectors::DiffractionEventCalibrateDetectors() :
      API::Algorithm()
    {}

    /// Destructor
    DiffractionEventCalibrateDetectors::~DiffractionEventCalibrateDetectors()
    {}



/**
 * The gsl_costFunction is optimized by GSL simplex
 * @param v :: vector containing center position and rotations
 * @param params :: names of detector, workspace, and instrument
 */

  static double gsl_costFunction(const gsl_vector *v, void *params)
  {
    double x, y, z, rotx, roty, rotz;
    std::string detname, inname, outname, peakOpt, rb_param;
    std::string *p = (std::string *)params;
    detname = p[0];
    inname = p[1];
    outname = p[2];
    peakOpt = p[3];
    rb_param = p[4];
    x = gsl_vector_get(v, 0);
    y = gsl_vector_get(v, 1);
    z = gsl_vector_get(v, 2);
    rotx = gsl_vector_get(v, 3);
    roty = gsl_vector_get(v, 4);
    rotz = gsl_vector_get(v, 5);
    Mantid::Algorithms::DiffractionEventCalibrateDetectors u;
    return u.intensity(x, y, z, rotx, roty, rotz, detname, inname, outname, peakOpt, rb_param);
  }

/**
 * The movedetector function changes detector position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param rotx :: The rotation around the X-axis
 * @param roty :: The rotation around the Y-axis
 * @param rotz :: The rotation around the Z-axis
 * @param detname :: The detector name
 * @param inputW :: The workspace
 */

  void DiffractionEventCalibrateDetectors::movedetector(double x, double y, double z, double rotx, double roty, double rotz, std::string detname, MatrixWorkspace_sptr inputW)
  {

    IAlgorithm_sptr alg1 = createSubAlgorithm("MoveInstrumentComponent");
    alg1->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    alg1->setPropertyValue("ComponentName", detname);
    //Move in cm for small shifts
    alg1->setProperty("X", x*0.01);
    alg1->setProperty("Y", y*0.01);
    alg1->setProperty("Z", z*0.01);
    alg1->setPropertyValue("RelativePosition", "1");
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
    algx->setProperty("X", 1.0);
    algx->setProperty("Y", 0.0);
    algx->setProperty("Z", 0.0);
    algx->setProperty("Angle", rotx);
    algx->setPropertyValue("RelativeRotation", "1");
    try
    {
      algx->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run RotateInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing RotateInstrumentComponent as a sub algorithm.");
    }

    IAlgorithm_sptr algy = createSubAlgorithm("RotateInstrumentComponent");
    algy->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    algy->setPropertyValue("ComponentName", detname);
    algy->setProperty("X", 0.0);
    algy->setProperty("Y", 1.0);
    algy->setProperty("Z", 0.0);
    algy->setProperty("Angle", roty);
    algy->setPropertyValue("RelativeRotation", "1");
    try
    {
      algy->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run RotateInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing RotateInstrumentComponent as a sub algorithm.");
    }

    IAlgorithm_sptr algz = createSubAlgorithm("RotateInstrumentComponent");
    algz->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    algz->setPropertyValue("ComponentName", detname);
    algz->setProperty("X", 0.0);
    algz->setProperty("Y", 0.0);
    algz->setProperty("Z", 1.0);
    algz->setProperty("Angle", rotz);
    algz->setPropertyValue("RelativeRotation", "1");
    try
    {
      algz->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run RotateInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing RotateInstrumentComponent as a sub algorithm.");
    }
  }
/**
 * The intensity function calculates the intensity as a function of detector position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param rotx :: The rotation around the X-axis
 * @param roty :: The rotation around the Y-axis
 * @param rotz :: The rotation around the Z-axis
 * @param detname :: The detector name
 * @param inname :: The workspace name
 * @param outname :: The workspace name
 * @param peakOpt :: Location of optimized peak
 * @param rb_param :: Bin boundary string
 */

  double DiffractionEventCalibrateDetectors::intensity(double x, double y, double z, double rotx, double roty, double rotz, std::string detname, std::string inname, std::string outname, std::string peakOpt, std::string rb_param)
  {

    MatrixWorkspace_sptr inputW = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve(inname));

    movedetector(x, y, z, rotx, roty, rotz, detname, inputW);
    IAlgorithm_sptr alg2 = createSubAlgorithm("CreateCalFileByNames");
    alg2->setProperty<MatrixWorkspace_sptr>("InstrumentWorkspace", inputW);
    std::string outputFile;
    outputFile = detname+".cal";
    alg2->setPropertyValue("GroupingFileName", outputFile);
    alg2->setPropertyValue("GroupNames", detname);
    try
    {
      alg2->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run CreateCalFileByNames sub-algorithm.");
      throw std::runtime_error("Error while executing CreateCalFileByNames as a sub algorithm.");
    }

    IAlgorithm_sptr alg3 = createSubAlgorithm("ConvertUnits");
    alg3->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
    alg3->setPropertyValue("OutputWorkspace", outname);
    //alg3->setPropertyValue("CalibrationFile", outputFile);
    alg3->setPropertyValue("Target","dSpacing");
    try
    {
      alg3->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run ConvertUnits sub-algorithm");
      throw std::runtime_error("Error while executing ConvertUnits as a sub algorithm.");
    }
    MatrixWorkspace_sptr outputW=alg3->getProperty("OutputWorkspace");

    IAlgorithm_sptr alg4 = createSubAlgorithm("DiffractionFocussing");
    alg4->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputW);
    alg4->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputW);
    alg4->setPropertyValue("GroupingFileName", outputFile);
    try
    {
      alg4->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run DiffractionFocussing sub-algorithm");
      throw std::runtime_error("Error while executing DiffractionFocussing as a sub algorithm.");
    }
    outputW=alg4->getProperty("OutputWorkspace");
    //Remove file
    Poco::File(outputFile).remove();

    IAlgorithm_sptr alg5 = createSubAlgorithm("Rebin");
    alg5->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputW);
    alg5->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputW);
    alg5->setPropertyValue("Params", rb_param);
    try
    {
      alg5->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run Rebin sub-algorithm");
      throw std::runtime_error("Error while executing Rebin as a sub algorithm.");
    }
    outputW=alg5->getProperty("OutputWorkspace");


  // Find point of peak centre
    const MantidVec & yValues = outputW->readY(0);
    MantidVec::const_iterator it = std::max_element(yValues.begin(), yValues.end());
    double peakHeight = *it;
    if(peakHeight == 0)return -0.000;
    double peakLoc = outputW->readX(0)[it - yValues.begin()];

    IAlgorithm_sptr fit_alg;
    try
    {
      //set the subalgorithm no to log as this will be run once per spectra
      fit_alg = createSubAlgorithm("Fit",-1,-1,false);
    } catch (Exception::NotFoundError&)
    {
      g_log.error("Can't locate Fit algorithm");
      throw ;
    }
    fit_alg->setProperty("InputWorkspace",outputW);
    fit_alg->setProperty("WorkspaceIndex",0);
    fit_alg->setProperty("StartX",outputW->readX(0)[0]);
    fit_alg->setProperty("EndX",outputW->readX(0)[outputW->blocksize()]);
    fit_alg->setProperty("MaxIterations",200);
    fit_alg->setProperty("Output","fit");
    std::ostringstream fun_str;
    fun_str << "name=Gaussian,Height="<<peakHeight<<",Sigma=0.01,PeakCentre="<<peakLoc;
    fit_alg->setProperty("Function",fun_str.str());

    try
    {
      fit_alg->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.error("Unable to successfully run Fit sub-algorithm");
      throw;
    }

    if ( ! fit_alg->isExecuted() )
    {
      g_log.error("Unable to successfully run Fit sub-algorithm");
      throw std::runtime_error("Unable to successfully run Fit sub-algorithm");
    }

    std::vector<double> params = fit_alg->getProperty("Parameters");
    peakHeight = params[0];
    peakLoc = params[1];

    movedetector(-x, -y, -z, -rotx, -roty, -rotz, detname, inputW);

    //Optimize C/peakheight + |peakLoc-peakOpt|  where C is scaled by number of events
    EventWorkspace_const_sptr inputE = boost::dynamic_pointer_cast<const EventWorkspace>( inputW );
    return (inputE->getNumberEvents()/1.e6)/peakHeight+std::fabs(peakLoc-boost::lexical_cast<double>(peakOpt));
}
  /** Initialisation method
  */
  void DiffractionEventCalibrateDetectors::init()
  {
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
                            "The workspace containing the geometry to be calibrated." );

    declareProperty("Params", "",
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Use bin boundaries close to peak you wish to maximize.\n"
        "Negative width values indicate logarithmic binning.");

    BoundedValidator<int>* mustBePositive = new BoundedValidator<int>();
    declareProperty("MaxIterations", 10, mustBePositive,
      "Stop after this number of iterations if a good fit is not found" );

    BoundedValidator<double>* dblmustBePositive = new BoundedValidator<double>();
    declareProperty("LocationOfPeakToOptimize", 2.0308, dblmustBePositive,
      "Optimize this location of peak by moving detectors" );

    declareProperty(new API::FileProperty("DetCalFilename", "", API::FileProperty::Save, ".DetCal"), "The output filename of the ISAW DetCal file");

    declareProperty( new PropertyWithValue<std::string>("BankName", "", Direction::Input),
    "Optional: To only calibrate one bank. Any bank whose name does not match the given string will have no events.");

    // Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    return;
  }


  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void DiffractionEventCalibrateDetectors::exec()
  {
    // Try to retrieve optional properties
    const int maxIterations = getProperty("MaxIterations");
    const double peakOpt = getProperty("LocationOfPeakToOptimize");

    // Get the input workspace
    MatrixWorkspace_const_sptr matrixInWS = getProperty("InputWorkspace");
    EventWorkspace_const_sptr inputW = boost::dynamic_pointer_cast<const EventWorkspace>( matrixInWS );
    if (!inputW)
      throw std::invalid_argument("InputWorkspace should be an EventWorkspace.");

     // retrieve the properties
    const std::string rb_params=getProperty("Params");

    //Get some stuff from the input workspace
    IInstrument_sptr inst = inputW->getInstrument();
    if (!inst)
      throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

    //Build a list of Rectangular Detectors
    std::vector<boost::shared_ptr<RectangularDetector> > detList;
    // --------- Loading only one bank ----------------------------------
    std::string onebank = getProperty("BankName");
    bool doOneBank = (onebank != "");
    for (int i=0; i < inst->nelements(); i++)
    {
      boost::shared_ptr<RectangularDetector> det;
      boost::shared_ptr<ICompAssembly> assem;

      det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
      if (det) 
      {
        if (det->getName().compare(onebank) == 0) detList.push_back(det);
        if (!doOneBank) detList.push_back(det);
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
               if (det->getName().compare(onebank) == 0) detList.push_back(det);
               if (!doOneBank) detList.push_back(det);
            }
          }
        }
      }
    }

    // set-up minimizer

    std::string inname = getProperty("InputWorkspace");
    std::string outname = inname+"2"; //getProperty("OutputWorkspace");

    IAlgorithm_sptr algS = createSubAlgorithm("Sort");
    algS->setPropertyValue("InputWorkspace",inname);
    algS->setPropertyValue("SortBy", "Time of Flight");
    try
    {
      algS->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run Sort sub-algorithm");
      throw std::runtime_error("Error while executing Sort as a sub algorithm.");
    }
    matrixInWS=algS->getProperty("InputWorkspace");

    //Write DetCal File
    double baseX,baseY,baseZ,upX,upY,upZ;

    std::string filename=getProperty("DetCalFilename");
    std::fstream outfile;
    outfile.open(filename.c_str(), std::ios::out);

    if(detList.size() > 1) 
    {
      outfile << "#\n";
      outfile << "#  Mantid Optimized .DetCal file for SNAP with TWO detector panels\n";
      outfile << "#  Old Panel, nominal size and distance at -90 degrees.\n";
      outfile << "#  New Panel, nominal size and distance at +90 degrees.\n";
      outfile << "#\n";
      outfile << "# Lengths are in centimeters.\n";
      outfile << "# Base and up give directions of unit vectors for a local\n";
      outfile << "# x,y coordinate system on the face of the detector.\n";
      outfile << "#\n";
      std::time_t current_t = DateAndTime::get_current_time().to_time_t() ;
      std::tm * current = gmtime( &current_t );
      outfile << "# "<<asctime (current) <<"\n";
      outfile << "#\n";
      outfile << "6         L1     T0_SHIFT\n";
      IObjComponent_const_sptr source = inst->getSource();
      IObjComponent_const_sptr sample = inst->getSample();
      outfile << "7  "<<source->getDistance(*sample)*100<<"            0\n";
      outfile << "4 DETNUM  NROWS  NCOLS  WIDTH   HEIGHT   DEPTH   DETD   CenterX   CenterY   CenterZ    BaseX    BaseY    BaseZ      UpX      UpY      UpZ\n";
    }

    Progress prog(this,0.0,1.0,detList.size());
    //omp_set_nested(1);
    //PARALLEL_FOR1(inputW)
    for (int det=0; det < static_cast<int>(detList.size()); det++)
    {
      //PARALLEL_START_INTERUPT_REGION
      std::string par[5];
      par[0]=detList[det]->getName();
      par[1]=inname;
      par[2]=outname;
      std::ostringstream strpeakOpt;
      strpeakOpt<<peakOpt;
      par[3]=strpeakOpt.str();
      par[4]=rb_params;
      const gsl_multimin_fminimizer_type *T =
      gsl_multimin_fminimizer_nmsimplex;
      gsl_multimin_fminimizer *s = NULL;
      gsl_vector *ss, *x;
      gsl_multimin_function minex_func;

      // finally do the fitting

      int nopt = 6;
      int iter = 0;
      int status = 0;
      double size;
 
      /* Starting point */
      x = gsl_vector_alloc (nopt);
      gsl_vector_set (x, 0, 0.0);
      gsl_vector_set (x, 1, 0.0);
      gsl_vector_set (x, 2, 0.0);
      gsl_vector_set (x, 3, 0.0);
      gsl_vector_set (x, 4, 0.0);
      gsl_vector_set (x, 5, 0.0);

      /* Set initial step sizes to 0.1 */
      ss = gsl_vector_alloc (nopt);
      gsl_vector_set_all (ss, 0.1);

      /* Initialize method and iterate */
      minex_func.n = nopt;
      minex_func.f = &Mantid::Algorithms::gsl_costFunction;
      minex_func.params = &par;

      s = gsl_multimin_fminimizer_alloc (T, nopt);
      gsl_multimin_fminimizer_set (s, &minex_func, x, ss);

      do
      {
        iter++;
        status = gsl_multimin_fminimizer_iterate(s);

        if (status)
          break;

        size = gsl_multimin_fminimizer_size (s);
        status = gsl_multimin_test_size (size, 1e-2);

      }
      while (status == GSL_CONTINUE && iter < maxIterations && s->fval != -0.000 );

      // Output summary to log file
      if (s->fval != -0.000) movedetector(gsl_vector_get (s->x, 0), gsl_vector_get (s->x, 1), gsl_vector_get (s->x, 2),
         gsl_vector_get (s->x, 3), gsl_vector_get (s->x, 4), gsl_vector_get (s->x, 5), par[0], getProperty("InputWorkspace"));
      else 
      {
        gsl_vector_set (s->x, 0, 0.0);
        gsl_vector_set (s->x, 1, 0.0);
        gsl_vector_set (s->x, 2, 0.0);
        gsl_vector_set (s->x, 3, 0.0);
        gsl_vector_set (s->x, 4, 0.0);
        gsl_vector_set (s->x, 5, 0.0);
      }

      std::string reportOfDiffractionEventCalibrateDetectors = gsl_strerror(status);
      if (s->fval == -0.000) reportOfDiffractionEventCalibrateDetectors = "No events";

      g_log.information() << "Detector = " << det << "\n" <<
        "Method used = " << "Simplex" << "\n" <<
        "Iteration = " << iter << "\n" <<
        "Status = " << reportOfDiffractionEventCalibrateDetectors << "\n" <<
        "Minimize PeakLoc-" << peakOpt << " = " << s->fval << "\n";
      //Move in cm for small shifts
      g_log.information() << "Move (X)   = " << gsl_vector_get (s->x, 0)*0.01 << "  \n";
      g_log.information() << "Move (Y)   = " << gsl_vector_get (s->x, 1)*0.01 << "  \n";
      g_log.information() << "Move (Z)   = " << gsl_vector_get (s->x, 2)*0.01 << "  \n";
      g_log.information() << "Rotate (X) = " << gsl_vector_get (s->x, 3) << "  \n";
      g_log.information() << "Rotate (Y) = " << gsl_vector_get (s->x, 4) << "  \n";
      g_log.information() << "Rotate (Z) = " << gsl_vector_get (s->x, 5) << "  \n";


      Geometry::V3D CalCenter=V3D(gsl_vector_get (s->x, 0)*0.01,
        gsl_vector_get (s->x, 1)*0.01, gsl_vector_get (s->x, 2)*0.01);
      Geometry::V3D Center=detList[det]->getPos()+CalCenter;
      int pixmax = detList[det]->xpixels()-1;
      int pixmid = (detList[det]->ypixels()-1)/2;
      BoundingBox box;
      detList[det]->getAtXY(pixmax, pixmid)->getBoundingBox(box);
      baseX = box.xMax();
      baseY = box.yMax();
      baseZ = box.zMax();
      Geometry::V3D Base=V3D(baseX,baseY,baseZ)+CalCenter;
      pixmid = (detList[det]->xpixels()-1)/2;
      pixmax = detList[det]->ypixels()-1;
      detList[det]->getAtXY(pixmid, pixmax)->getBoundingBox(box);
      upX = box.xMax();
      upY = box.yMax();
      upZ = box.zMax();
      Geometry::V3D Up=V3D(upX,upY,upZ)+CalCenter;
      Base-=Center;
      Up-=Center;
      //Rotate around x
      baseX = Base[0];
      baseY = Base[1];
      baseZ = Base[2];
      double deg2rad=M_PI/180.0;
      double angle = gsl_vector_get (s->x, 3)*deg2rad;
      Base=V3D(baseX,baseY*cos(angle)-baseZ*sin(angle),
        baseY*sin(angle)+baseZ*cos(angle));
      upX = Up[0];
      upY = Up[1];
      upZ = Up[2];
      Up=V3D(upX,upY*cos(angle)-upZ*sin(angle),
        upY*sin(angle)+upZ*cos(angle));
      //Rotate around y
      baseX = Base[0];
      baseY = Base[1];
      baseZ = Base[2];
      angle = gsl_vector_get (s->x, 4)*deg2rad;
      Base=V3D(baseZ*sin(angle)+baseX*cos(angle),
        baseY,baseZ*cos(angle)-baseX*sin(angle));
      upX = Up[0];
      upY = Up[1];
      upZ = Up[2];
      Up=V3D(upZ*cos(angle)-upX*sin(angle),upY,
        upZ*sin(angle)+upX*cos(angle));
      //Rotate around z
      baseX = Base[0];
      baseY = Base[1];
      baseZ = Base[2];
      angle = gsl_vector_get (s->x, 5)*deg2rad;
      Base=V3D(baseX*cos(angle)-baseY*sin(angle),
        baseX*sin(angle)+baseY*cos(angle),baseZ);
      upX = Up[0];
      upY = Up[1];
      upZ = Up[2];
      Up=V3D(upX*cos(angle)-upY*sin(angle),
        upX*sin(angle)+upY*cos(angle),upZ);
      Base.normalize();
      Up.normalize();
      Center*=100.0;
      // << det+1  << "  " 
      outfile << "5  " 
       << detList[det]->getName().substr(4)  << "  " 
       << detList[det]->xpixels() << "  " 
       << detList[det]->ypixels() << "  " 
       << 100.0*detList[det]->xsize() << "  " 
       << 100.0*detList[det]->ysize() << "  " 
       << "0.2000" << "  " 
       << Center.norm() << "  " ;
      Center.write(outfile);
      outfile << "  ";
      Base.write(outfile);
      outfile << "  ";
      Up.write(outfile);
      outfile << "\n";

      // clean up dynamically allocated gsl stuff
      gsl_vector_free(x);
      gsl_vector_free(ss);
      gsl_multimin_fminimizer_free (s);
      prog.report();
      //PARALLEL_END_INTERUPT_REGION
    }
    //PARALLEL_CHECK_INTERUPT_REGION

    // Closing
    outfile.close();

    return;
  }


} // namespace Algorithm
} // namespace Mantid
