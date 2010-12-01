//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionEventCalibrateDetectors.h"
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
 * The gsl_costFunction is optimized by GSL simplex2
 * @param v vector containing center position and rotations
 * @param params names of detector, workspace, and instrument
 */

  static double gsl_costFunction(const gsl_vector *v, void *params)
  {
    double x, y, z, rotx, roty, rotz;
    std::string detname, inname, outname, instname;
    std::string *p = (std::string *)params;
    detname = p[0];
    inname = p[1];
    outname = p[2];
    instname = p[3];
    x = gsl_vector_get(v, 0);
    y = gsl_vector_get(v, 1);
    z = gsl_vector_get(v, 2);
    rotx = gsl_vector_get(v, 3);
    roty = gsl_vector_get(v, 4);
    rotz = gsl_vector_get(v, 5);
    Mantid::Algorithms::DiffractionEventCalibrateDetectors u;
    // To maximize intensity, minimize -intensity
    return -u.intensity(x, y, z, rotx, roty, rotz, detname, inname, outname, instname);
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

  double DiffractionEventCalibrateDetectors::intensity(double x, double y, double z, double rotx, double roty, double rotz, std::string detname, std::string inname, std::string outname, std::string instname)
  {

    MatrixWorkspace_sptr inputW = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve(inname));

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

    IAlgorithm_sptr alg2 = createSubAlgorithm("CreateCalFileByNames");
    alg2->setPropertyValue("InstrumentName", instname);
    std::string outputFile;
    outputFile = "DiffractionEventCalibrateDetectors.cal";
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
    alg5->setPropertyValue("Params", ".2,0.0002,10.");
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
    std::cout << x <<" "<< y <<" "<< z <<" "<< rotx <<" "<< roty <<" "<< rotz <<" "<<peakHeight <<" "<<peakLoc<<"\n";

    IAlgorithm_sptr alg6 = createSubAlgorithm("MoveInstrumentComponent");
    alg6->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    alg6->setPropertyValue("ComponentName", detname);
    alg6->setProperty("X", -x*0.01);
    alg6->setProperty("Y", -y*0.01);
    alg6->setProperty("Z", -z*0.01);
    alg6->setPropertyValue("RelativePosition", "1");
    try
    {
      alg6->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run MoveInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing MoveInstrumentComponent as a sub algorithm.");
    }

    IAlgorithm_sptr alg6x = createSubAlgorithm("RotateInstrumentComponent");
    alg6x->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    alg6x->setPropertyValue("ComponentName", detname);
    alg6x->setProperty("X", 1.0);
    alg6x->setProperty("Y", 0.0);
    alg6x->setProperty("Z", 0.0);
    alg6x->setProperty("Angle", -rotx);
    try
    {
      alg6x->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run RotateInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing RotateInstrumentComponent as a sub algorithm.");
    }

    IAlgorithm_sptr alg6y = createSubAlgorithm("RotateInstrumentComponent");
    alg6y->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    alg6y->setPropertyValue("ComponentName", detname);
    alg6y->setProperty("X", 0.0);
    alg6y->setProperty("Y", 1.0);
    alg6y->setProperty("Z", 0.0);
    alg6y->setProperty("Angle", -roty);
    try
    {
      alg6y->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run RotateInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing RotateInstrumentComponent as a sub algorithm.");
    }

    IAlgorithm_sptr alg6z = createSubAlgorithm("RotateInstrumentComponent");
    alg6z->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
    alg6z->setPropertyValue("ComponentName", detname);
    alg6z->setProperty("X", 0.0);
    alg6z->setProperty("Y", 0.0);
    alg6z->setProperty("Z", 1.0);
    alg6z->setProperty("Angle", -rotz);
    try
    {
      alg6z->execute();
    }
    catch (std::runtime_error&)
    {
      g_log.information("Unable to successfully run RotateInstrumentComponent sub-algorithm");
      throw std::runtime_error("Error while executing RotateInstrumentComponent as a sub algorithm.");
    }

    return peakHeight;
}
  /** Initialisation method
  */
  void DiffractionEventCalibrateDetectors::init()
  {
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
                            "The workspace containing the geometry to be calibrated." );
  /*declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm." );*/

    declareProperty("DetectorName","","Detector to move");

    BoundedValidator<int>* mustBePositive = new BoundedValidator<int>();
    declareProperty("MaxIterations", 500, mustBePositive,
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

    //Get some stuff from the input workspace
    IInstrument_sptr inst = inputW->getInstrument();
    if (!inst)
      throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

    // set-up minimizer

    std::string par[4];
    std::string detname = getProperty("DetectorName");
    std::string inname = getProperty("InputWorkspace");
    std::string outname = inname+"2"; //getProperty("OutputWorkspace");
    std::string instname = inst->getName();
    par[0]=detname;
    par[1]=inname;
    par[2]=outname;
    par[3]=instname;
    const gsl_multimin_fminimizer_type *T =
      gsl_multimin_fminimizer_nmsimplex2;
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

    Progress prog(this,0.0,1.0,maxIterations);
    do
    {
      iter++;
      status = gsl_multimin_fminimizer_iterate(s);

      if (status)
         break;

      size = gsl_multimin_fminimizer_size (s);
      status = gsl_multimin_test_size (size, 1e-2);
      prog.report();

      if (status == GSL_SUCCESS)
      {
           printf ("converged to minimum at\n");
      }

      printf ("%5d %10.3e %10.3e %10.3e %10.3e %10.3e %10.3e f() = %7.3f size = %.3f\n",
              status,
              gsl_vector_get (s->x, 0),
              gsl_vector_get (s->x, 1),
              gsl_vector_get (s->x, 2),
              gsl_vector_get (s->x, 3),
              gsl_vector_get (s->x, 4),
              gsl_vector_get (s->x, 5),
              s->fval, size);
    }
    while (status == GSL_CONTINUE && iter < maxIterations);

    // Output summary to log file

    std::string reportOfDiffractionEventCalibrateDetectors = gsl_strerror(status);

    /*g_log.information() << "Method used = " << "Simplex" << "\n" <<
      "Iteration = " << iter << "\n" <<
      "Status = " << reportOfDiffractionEventCalibrateDetectors << "\n" <<
      "Chi^2/DoF = " << s->fval << "\n";
    std::string parameterName[3] = {"X","Y","Z"};
    for (int i = 0; i < nopt; i++)
    {
      g_log.information() << parameterName[i] << " = " << gsl_vector_get (s->x, i) << "  \n";
    }*/


    // clean up dynamically allocated gsl stuff
    gsl_vector_free(x);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free (s);

    return;
  }


} // namespace Algorithm
} // namespace Mantid
