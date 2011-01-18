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
 * @param v vector containing center position and rotations
 * @param params names of detector, workspace, and instrument
 */

  static double gsl_costFunction(const gsl_vector *v, void *params)
  {
    double x, y, z, rotx, roty, rotz;
    std::string detname, inname, outname, instname, rb_param;
    std::string *p = (std::string *)params;
    detname = p[0];
    inname = p[1];
    outname = p[2];
    instname = p[3];
    rb_param = p[4];
    x = gsl_vector_get(v, 0);
    y = gsl_vector_get(v, 1);
    z = gsl_vector_get(v, 2);
    rotx = gsl_vector_get(v, 3);
    roty = gsl_vector_get(v, 4);
    rotz = gsl_vector_get(v, 5);
    Mantid::Algorithms::DiffractionEventCalibrateDetectors u;
    // To maximize intensity, minimize -intensity
    return -u.intensity(x, y, z, rotx, roty, rotz, detname, inname, outname, instname, rb_param);
  }

/**
 * The movedetector function changes detector position and angles
 * @param x The shift along the X-axis
 * @param y The shift along the Y-axis
 * @param z The shift along the Z-axis
 * @param rotx The rotation around the X-axis
 * @param roty The rotation around the Y-axis
 * @param rotz The rotation around the Z-axis
 * @param detname The detector name
 * @param inputW The workspace
 */

  void DiffractionEventCalibrateDetectors::movedetector(double x, double y, double z, double rotx, double roty, double rotz, std::string detname, MatrixWorkspace_sptr inputW)
  {

    IAlgorithm_sptr alg1 = createSubAlgorithm("MoveInstrumentComponent");
    alg1->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    alg1->setPropertyValue("ComponentName", detname);
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
 * @param x The shift along the X-axis
 * @param y The shift along the Y-axis
 * @param z The shift along the Z-axis
 * @param rotx The rotation around the X-axis
 * @param roty The rotation around the Y-axis
 * @param rotz The rotation around the Z-axis
 * @param detname The detector name
 * @param inname The workspace name
 * @param outname The workspace name
 * @param instname The instrument name
 */

  double DiffractionEventCalibrateDetectors::intensity(double x, double y, double z, double rotx, double roty, double rotz, std::string detname, std::string inname, std::string outname, std::string instname, std::string rb_param)
  {

    MatrixWorkspace_sptr inputW = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve(inname));

    movedetector(x, y, z, rotx, roty, rotz, detname, inputW);
    IAlgorithm_sptr alg2 = createSubAlgorithm("CreateCalFileByNames");
    alg2->setPropertyValue("InstrumentName", instname);
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
      g_log.information("Unable to successfully run CreateCalFileByNames sub-algorithm");
      throw std::runtime_error("Error while executing CreateCalFileByNames as a sub algorithm.");
    }

    IAlgorithm_sptr alg3 = createSubAlgorithm("AlignDetectors");
    alg3->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
    alg3->setPropertyValue("OutputWorkspace", outname);
    alg3->setPropertyValue("CalibrationFile", outputFile);
    //alg3->setPropertyValue("Target","dSpacing");
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
    const double peakHeight = *it;
    const double peakLoc = outputW->readX(0)[it - yValues.begin()];

    movedetector(-x, -y, -z, -rotx, -roty, -rotz, detname, inputW);

    return peakHeight;
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


    // set-up minimizer

    std::string inname = getProperty("InputWorkspace");
    std::string outname = inname+"2"; //getProperty("OutputWorkspace");
    std::string instname = inst->getName();
    PARALLEL_FOR1(matrixInWS)
    for (int det=0; det < detList.size(); det++)
    {
      PARALLEL_START_INTERUPT_REGION
      std::string par[5];
      par[0]=detList[det]->getName();
      par[1]=inname;
      par[2]=outname;
      par[3]=instname;
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

      //Progress prog(this,0.0,1.0,maxIterations);
      do
      {
        iter++;
        status = gsl_multimin_fminimizer_iterate(s);

        if (status)
          break;

        size = gsl_multimin_fminimizer_size (s);
        status = gsl_multimin_test_size (size, 1e-2);
        //prog.report();

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

      g_log.information() << "Detector = " << det << "\n" <<
        "Method used = " << "Simplex" << "\n" <<
        "Iteration = " << iter << "\n" <<
        "Status = " << reportOfDiffractionEventCalibrateDetectors << "\n" <<
        "Minimize -PeakHeight = " << s->fval << "\n";
      g_log.information() << "Move (X)   = " << gsl_vector_get (s->x, 0) << "  \n";
      g_log.information() << "Move (Y)   = " << gsl_vector_get (s->x, 1) << "  \n";
      g_log.information() << "Move (Z)   = " << gsl_vector_get (s->x, 2) << "  \n";
      g_log.information() << "Rotate (X) = " << gsl_vector_get (s->x, 3) << "  \n";
      g_log.information() << "Rotate (Y) = " << gsl_vector_get (s->x, 4) << "  \n";
      g_log.information() << "Rotate (Z) = " << gsl_vector_get (s->x, 5) << "  \n";


      // clean up dynamically allocated gsl stuff
      gsl_vector_free(x);
      gsl_vector_free(ss);
      gsl_multimin_fminimizer_free (s);
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    return;
  }


} // namespace Algorithm
} // namespace Mantid
