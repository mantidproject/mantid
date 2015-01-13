#include "MantidCrystal/OptimizeExtinctionParameters.h"
#include "MantidCrystal/GSLFunctions.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
// DECLARE_ALGORITHM(OptimizeExtinctionParameters)

using namespace Kernel;
using namespace API;
using std::size_t;
using namespace DataObjects;

/// Constructor
OptimizeExtinctionParameters::OptimizeExtinctionParameters() {
  m_pointGroups = getAllPointGroups();
}

/// Destructor
OptimizeExtinctionParameters::~OptimizeExtinctionParameters() {}

static double gsl_costFunction(const gsl_vector *v, void *params) {
  std::string *p = (std::string *)params;
  std::string inname = p[0];
  std::string corrOption = p[1];
  std::string pointOption = p[2];
  std::string tofParams = p[3];
  std::vector<double> tofParam =
      Kernel::VectorHelper::splitStringIntoVector<double>(tofParams);
  double rcrystallite = tofParam[1];
  double mosaic = tofParam[2];
  if (corrOption.compare(5, 2, "II") == 0)
    rcrystallite = gsl_vector_get(v, 0);
  else
    mosaic = gsl_vector_get(v, 0);
  if (v->size > 1)
    rcrystallite = gsl_vector_get(v, 1);
  Mantid::Algorithms::OptimizeExtinctionParameters u;
  return u.fitMosaic(mosaic, rcrystallite, inname, corrOption, pointOption,
                     tofParams);
}

//-----------------------------------------------------------------------------------------
/** Initialisation method. Declares properties to be used in algorithm.
 */
void OptimizeExtinctionParameters::init() {

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace", "",
                                                        Direction::InOut),
                  "An input PeaksWorkspace with an instrument.");
  std::vector<std::string> corrOptions;
  corrOptions.push_back("Type I Zachariasen");
  corrOptions.push_back("Type I Gaussian");
  corrOptions.push_back("Type I Lorentzian");
  corrOptions.push_back("Type II Zachariasen");
  corrOptions.push_back("Type II Gaussian");
  corrOptions.push_back("Type II Lorentzian");
  corrOptions.push_back("Type I&II Zachariasen");
  corrOptions.push_back("Type I&II Gaussian");
  corrOptions.push_back("Type I&II Lorentzian");
  corrOptions.push_back("None, Scaling Only");
  declareProperty("ExtinctionCorrectionType", corrOptions[0],
                  boost::make_shared<StringListValidator>(corrOptions),
                  "Select the type of extinction correction.");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Cell", 255.0, "Unit Cell Volume (Angstroms^3)");
  declareProperty("Mosaic", 0.262, "Mosaic Spread (FWHM) (Degrees)",
                  Direction::InOut);
  declareProperty("RCrystallite", 6.0,
                  "Becker-Coppens Crystallite Radius (micron)",
                  Direction::InOut);
  std::vector<std::string> propOptions;
  for (size_t i = 0; i < m_pointGroups.size(); ++i)
    propOptions.push_back(m_pointGroups[i]->getName());
  declareProperty("PointGroup", propOptions[0],
                  boost::make_shared<StringListValidator>(propOptions),
                  "Which point group applies to this crystal?");
  declareProperty("OutputChi2", 0.0, Direction::Output);

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();
}

//-----------------------------------------------------------------------------------------
/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 */
void OptimizeExtinctionParameters::exec() {
  std::string par[4];
  std::string inname = getProperty("InputWorkspace");
  par[0] = inname;
  std::string type = getProperty("ExtinctionCorrectionType");
  par[1] = type;
  std::string group = getProperty("PointGroup");
  par[2] = group;
  PeaksWorkspace_sptr ws = getProperty("InputWorkspace");

  double mosaic = getProperty("Mosaic");
  double cell = getProperty("Cell");
  double r_crystallite = getProperty("RCrystallite");
  std::ostringstream strwi;
  strwi << cell << "," << r_crystallite << "," << mosaic;
  par[3] = strwi.str();

  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;
  gsl_multimin_fminimizer *s = NULL;
  gsl_vector *ss, *x;
  gsl_multimin_function minex_func;

  // finally do the fitting

  size_t nopt = 1;
  if (type.compare(7, 2, "II") == 0)
    nopt = 2;
  size_t iter = 0;
  int status = 0;

  /* Starting point */
  x = gsl_vector_alloc(nopt);
  if (type.compare(5, 2, "II") == 0)
    gsl_vector_set(x, 0, r_crystallite);
  else
    gsl_vector_set(x, 0, mosaic);
  if (nopt > 1)
    gsl_vector_set(x, 1, r_crystallite);

  /* Set initial step sizes to 0.001 */
  ss = gsl_vector_alloc(nopt);
  gsl_vector_set_all(ss, 0.001);

  /* Initialize method and iterate */
  minex_func.n = nopt;
  minex_func.f = &Mantid::Algorithms::gsl_costFunction;
  minex_func.params = &par;

  s = gsl_multimin_fminimizer_alloc(T, nopt);
  gsl_multimin_fminimizer_set(s, &minex_func, x, ss);

  do {
    iter++;
    status = gsl_multimin_fminimizer_iterate(s);
    if (status)
      break;

    double size = gsl_multimin_fminimizer_size(s);
    status = gsl_multimin_test_size(size, 1e-4);

  } while (status == GSL_CONTINUE && iter < 500);

  // Output summary to log file
  std::string reportOfDiffractionEventCalibrateDetectors = gsl_strerror(status);
  // g_log.debug() <<
  if (type.compare(5, 2, "II") == 0)
    r_crystallite = gsl_vector_get(s->x, 0);
  else
    mosaic = gsl_vector_get(s->x, 0);
  if (nopt > 1)
    r_crystallite = gsl_vector_get(s->x, 1);
  std::cout << " Method used = "
            << " Simplex"
            << " Iteration = " << iter
            << " Status = " << reportOfDiffractionEventCalibrateDetectors
            << " Minimize Sum = " << s->fval << " Mosaic   = " << mosaic
            << " RCrystallite  = " << r_crystallite << "  \n";
  gsl_vector_free(x);
  gsl_vector_free(ss);
  gsl_multimin_fminimizer_free(s);
  setProperty("Mosaic", mosaic);
  setProperty("RCrystallite", r_crystallite);
  setProperty("OutputChi2", s->fval);
}

//-----------------------------------------------------------------------------------------
/**
 * Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
 * @param mosaic
 * @param rcrystallite
 * @param inname
 * @param corrOption
 * @param pointOption
 * @param tofParams
 * @return
 */
double OptimizeExtinctionParameters::fitMosaic(
    double mosaic, double rcrystallite, std::string inname,
    std::string corrOption, std::string pointOption, std::string tofParams) {
  PeaksWorkspace_sptr inputW = boost::dynamic_pointer_cast<PeaksWorkspace>(
      AnalysisDataService::Instance().retrieve(inname));
  std::vector<double> tofParam =
      Kernel::VectorHelper::splitStringIntoVector<double>(tofParams);
  if (mosaic < 0.0 || rcrystallite < 0.0)
    return 1e300;

  API::IAlgorithm_sptr tofextinction =
      createChildAlgorithm("TOFExtinction", 0.0, 0.2);
  tofextinction->setProperty("InputWorkspace", inputW);
  tofextinction->setProperty("OutputWorkspace", "tmp");
  tofextinction->setProperty("ExtinctionCorrectionType", corrOption);
  tofextinction->setProperty<double>("Mosaic", mosaic);
  tofextinction->setProperty<double>("Cell", tofParam[0]);
  tofextinction->setProperty<double>("RCrystallite", rcrystallite);
  tofextinction->executeAsChildAlg();
  PeaksWorkspace_sptr peaksW = tofextinction->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr sorthkl = createChildAlgorithm("SortHKL", 0.0, 0.2);
  sorthkl->setProperty("InputWorkspace", peaksW);
  sorthkl->setProperty("OutputWorkspace", peaksW);
  sorthkl->setProperty("PointGroup", pointOption);
  sorthkl->executeAsChildAlg();
  double Chisq = sorthkl->getProperty("OutputChi2");
  std::cout << mosaic << "  " << rcrystallite << "  " << Chisq << "\n";
  return Chisq;
}

} // namespace Algorithm
} // namespace Mantid
