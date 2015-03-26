#include "MantidCurveFitting/EstimatePeakErrors.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidCurveFitting/PeakParameterFunction.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace CurveFitting {
using namespace API;
using namespace Kernel;
using namespace std;

// Subscription
DECLARE_ALGORITHM(EstimatePeakErrors)

//--------------------------------------------------------------------------------------------------------
// Public members
//--------------------------------------------------------------------------------------------------------

/// Default constructor
EstimatePeakErrors::EstimatePeakErrors() : Algorithm() {}

/// Summary of algorithms purpose
const std::string EstimatePeakErrors::summary() const {
  return "Calculates error estimates for peak parameters: "
         "centre, height, FWHM and intensity.";
}

const std::string EstimatePeakErrors::name() const {
  return "EstimatePeakErrors";
}

int EstimatePeakErrors::version() const { return 1; }

const std::string EstimatePeakErrors::category() const {
  return "Optimization";
}

//--------------------------------------------------------------------------------------------------------
// Private members
//--------------------------------------------------------------------------------------------------------
namespace {

/// Calculate a Jacobian of transformations from the normal function's
/// parameters to the 4 general peak parameters: centre, height, FWHM and
/// intensity (integral).
/// Also calculate the values for the peak parameters.
/// @param peak :: The function for which the Jacobian to be calculated.
/// @param centre :: Output receiving value of peak centre.
/// @param height :: Output receiving value of peak height.
/// @param fwhm :: Output receiving value of peak FWHM.
/// @param intensity :: Output receiving value of peak intensity.
GSLMatrix makeJacobian(IPeakFunction &peak, double &centre, double &height,
                       double &fwhm, double &intensity) {
  GSLMatrix jacobian(4, peak.nParams());
  centre = peak.centre();
  height = peak.height();
  fwhm = peak.fwhm();
  intensity = peak.intensity();
  for (size_t ip = 0; ip < peak.nParams(); ++ip) {
    double p = peak.getParameter(ip);
    double dp = 1e-9;
    if (p != 0.0) {
      dp *= p;
    }
    peak.setParameter(ip, p + dp);
    jacobian.set(0, ip, (peak.centre() - centre) / dp);
    jacobian.set(1, ip, (peak.height() - height) / dp);
    jacobian.set(2, ip, (peak.fwhm() - fwhm) / dp);
    jacobian.set(3, ip, (peak.intensity() - intensity) / dp);
    peak.setParameter(ip, p);
  }
  return jacobian;
}

/// Calculate the errors for a peak and add them to the result table.
/// @param peak :: A function for which errors are calculated.
/// @param results :: The table with results
/// @param covariance :: The covariance matrix for the parameters of the peak.
/// @param prefix :: A prefix for the parameter names.
void calculatePeakValues(IPeakFunction &peak, ITableWorkspace &results,
                         const GSLMatrix covariance,
                         const std::string &prefix) {
  double centre, height, fwhm, intensity;
  GSLMatrix J = makeJacobian(peak, centre, height, fwhm, intensity);
  // CHECK_OUT_GSL_MATRIX("J=", J);

  GSLMatrix JCJ = J * covariance * Tr(J);
  // CHECK_OUT_GSL_MATRIX("JCJ=", JCJ);

  TableRow row = results.appendRow();
  row << prefix + "Centre" << centre << sqrt(JCJ.get(0, 0));
  row = results.appendRow();
  row << prefix + "Height" << height << sqrt(JCJ.get(1, 1));
  row = results.appendRow();
  row << prefix + "FWHM" << fwhm << sqrt(JCJ.get(2, 2));
  row = results.appendRow();
  row << prefix + "Intensity" << intensity << sqrt(JCJ.get(3, 3));
}
}

/// Initialize
void EstimatePeakErrors::init() {

  declareProperty(
      new FunctionProperty("Function"),
      "Fitting function containing peaks. Must have a covariance matrix attached.");

  declareProperty(
      new API::WorkspaceProperty<API::ITableWorkspace>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the TableWorkspace with the output values and errors.");
}

/// Execute
void EstimatePeakErrors::exec() {

  IFunction_sptr function = getProperty("Function");

  ITableWorkspace_sptr results =
      WorkspaceFactory::Instance().createTable("TableWorkspace");
  results->addColumn("str", "Parameter");
  results->addColumn("double", "Value");
  results->addColumn("double", "Error");

  auto matrix = function->getCovarianceMatrix();
  if ( !matrix )
  {
    g_log.warning() << "Function doesn't have covariance matrix." << std::endl;
    setProperty("OutputWorkspace", results);
    return;
  }

  IPeakFunction *peak = dynamic_cast<IPeakFunction *>(function.get());

  if (peak) {
    GSLMatrix covariance(*matrix);
    calculatePeakValues(*peak, *results, covariance, "");
  } else {
    CompositeFunction *cf = dynamic_cast<CompositeFunction *>(function.get());
    if (cf) {
      size_t ip = 0;
      for (size_t i = 0; i < cf->nFunctions(); ++i) {
        IFunction *fun = cf->getFunction(i).get();
        size_t np = fun->nParams();
        IPeakFunction *peak = dynamic_cast<IPeakFunction *>(fun);
        if (peak) {
          std::string prefix = "f" + boost::lexical_cast<std::string>(i) + ".";
          GSLMatrix covariance(*matrix, ip, ip, np, np);
          calculatePeakValues(*peak, *results, covariance, prefix);
        }
        ip += np;
      }
    } else {
      g_log.warning() << "Function has no peaks." << std::endl;
    }
  }

  setProperty("OutputWorkspace", results);
}

} // namespace CurveFitting
} // namespace Mantid
