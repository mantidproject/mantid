// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PDFFourierTransform.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>
#include <sstream>

namespace Mantid::Algorithms {

using std::string;
using namespace HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDFFourierTransform)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace DataObjects;

namespace { // anonymous namespace
/// Crystalline PDF
const string BIG_G_OF_R("G(r)");
/// Liquids PDF
const string LITTLE_G_OF_R("g(r)");
/// Radial distribution function
const string RDF_OF_R("RDF(r)");

/// Normalized intensity
const string S_OF_Q("S(Q)");
/// Asymptotes to zero
const string S_OF_Q_MINUS_ONE("S(Q)-1");
/// Kernel of the Fourier transform
const string Q_S_OF_Q_MINUS_ONE("Q[S(Q)-1]");

constexpr double TWO_OVER_PI(2. / M_PI);
} // namespace

const std::string PDFFourierTransform::name() const { return "PDFFourierTransform"; }

int PDFFourierTransform::version() const { return 1; }

const std::string PDFFourierTransform::category() const { return "Diffraction\\Utility"; }

/** Initialize the algorithm's properties.
 */
void PDFFourierTransform::init() {
  auto uv = std::make_shared<API::WorkspaceUnitValidator>("MomentumTransfer");

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, uv),
                  S_OF_Q + ", " + S_OF_Q_MINUS_ONE + ", or " + Q_S_OF_Q_MINUS_ONE);
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Result paired-distribution function");

  // Set up input data type
  std::vector<std::string> inputTypes;
  inputTypes.emplace_back(S_OF_Q);
  inputTypes.emplace_back(S_OF_Q_MINUS_ONE);
  inputTypes.emplace_back(Q_S_OF_Q_MINUS_ONE);
  declareProperty("InputSofQType", S_OF_Q, std::make_shared<StringListValidator>(inputTypes),
                  "To identify whether input function");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty("Qmin", EMPTY_DBL(), mustBePositive,
                  "Minimum Q in S(Q) to calculate in Fourier transform (optional).");
  declareProperty("Qmax", EMPTY_DBL(), mustBePositive,
                  "Maximum Q in S(Q) to calculate in Fourier transform. (optional)");
  declareProperty("Filter", false, "Set to apply Lorch function filter to the input");

  // Set up output data type
  std::vector<std::string> outputTypes;
  outputTypes.emplace_back(BIG_G_OF_R);
  outputTypes.emplace_back(LITTLE_G_OF_R);
  outputTypes.emplace_back(RDF_OF_R);
  declareProperty("PDFType", BIG_G_OF_R, std::make_shared<StringListValidator>(outputTypes),
                  "Type of output PDF including G(r)");

  declareProperty("DeltaR", EMPTY_DBL(), mustBePositive,
                  "Step size of r of G(r) to calculate.  Default = "
                  ":math:`\\frac{\\pi}{Q_{max}}`.");
  declareProperty("Rmax", 20., mustBePositive, "Maximum r for G(r) to calculate.");
  declareProperty("rho0", EMPTY_DBL(), mustBePositive,
                  "Average number density used for g(r) and RDF(r) conversions (optional)");

  string recipGroup("Reciprocal Space");
  setPropertyGroup("InputSofQType", recipGroup);
  setPropertyGroup("Qmin", recipGroup);
  setPropertyGroup("Qmax", recipGroup);
  setPropertyGroup("Filter", recipGroup);

  string realGroup("Real Space");
  setPropertyGroup("PDFType", realGroup);
  setPropertyGroup("DeltaR", realGroup);
  setPropertyGroup("Rmax", realGroup);
  setPropertyGroup("rho0", realGroup);
}

std::map<string, string> PDFFourierTransform::validateInputs() {
  std::map<string, string> result;

  double Qmin = getProperty("Qmin");
  double Qmax = getProperty("Qmax");
  if ((!isEmpty(Qmin)) && (!isEmpty(Qmax)))
    if (Qmax <= Qmin)
      result["Qmax"] = "Must be greater than Qmin";

  // check for null pointers - this is to protect against workspace groups
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS) {
    return result;
  }

  if (inputWS->getNumberHistograms() != 1) {
    result["InputWorkspace"] = "Input workspace must have only one spectrum";
  }

  return result;
}

size_t PDFFourierTransform::determineQminIndex(const std::vector<double> &Q, const std::vector<double> &FofQ) {
  double qmin = getProperty("Qmin");

  // check against available Q-range
  if (isEmpty(qmin)) {
    qmin = Q.front();
  } else if (qmin < Q.front()) {
    g_log.information("Specified Qmin < range of data. Adjusting to data range.");
    qmin = Q.front();
  }

  // get index for the Qmin from the Q-range
  auto q_iter = std::upper_bound(Q.begin(), Q.end(), qmin);
  size_t qmin_index = std::distance(Q.begin(), q_iter);
  if (qmin_index == 0)
    qmin_index += 1; // so there doesn't have to be a check in integration loop

  // go to first non-nan value
  q_iter = std::find_if(std::next(FofQ.begin(), qmin_index), FofQ.end(), static_cast<bool (*)(double)>(std::isnormal));
  size_t first_normal_index = std::distance(FofQ.begin(), q_iter);
  if (first_normal_index > qmin_index) {
    g_log.information("Specified Qmin where data is nan/inf. Adjusting to data range.");
    qmin_index = first_normal_index;
  }

  return qmin_index;
}

size_t PDFFourierTransform::determineQmaxIndex(const std::vector<double> &Q, const std::vector<double> &FofQ) {
  double qmax = getProperty("Qmax");

  // check against available Q-range
  if (isEmpty(qmax)) {
    qmax = Q.back();
  } else if (qmax > Q.back()) {
    g_log.information() << "Specified Qmax > range of data. Adjusting to data range.\n";
    qmax = Q.back();
  }

  // get pointers for the data range
  auto q_iter = std::lower_bound(Q.begin(), Q.end(), qmax);
  size_t qmax_index = std::distance(Q.begin(), q_iter);

  // go to first non-nan value
  auto q_back_iter = std::find_if(FofQ.rbegin(), FofQ.rend(), static_cast<bool (*)(double)>(std::isnormal));
  size_t first_normal_index = FofQ.size() - std::distance(FofQ.rbegin(), q_back_iter) - 1;
  if (first_normal_index < qmax_index) {
    g_log.information("Specified Qmax where data is nan/inf. Adjusting to data range.");
    qmax_index = first_normal_index;
  }

  return qmax_index;
}

double PDFFourierTransform::determineRho0() {
  double rho0 = getProperty("rho0");

  if (isEmpty(rho0)) {
    API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

    const Kernel::Material &material = inputWS->sample().getMaterial();
    double materialDensity = material.numberDensity();

    if (!isEmpty(materialDensity) && materialDensity > 0)
      rho0 = materialDensity;
    else
      rho0 = 1.;
  }

  return rho0;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PDFFourierTransform::exec() {
  // get input data
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  auto inputQ = inputWS->x(0).rawData();                  //  x for input
  HistogramData::HistogramDx inputDQ(inputQ.size(), 0.0); // dx for input
  if (inputWS->sharedDx(0))
    inputDQ = inputWS->dx(0);
  auto inputFOfQ = inputWS->y(0).rawData();  //  y for input
  auto inputDfOfQ = inputWS->e(0).rawData(); // dy for input

  // transform input data into Q/MomentumTransfer
  const std::string inputXunit = inputWS->getAxis(0)->unit()->unitID();
  if (inputXunit == "MomentumTransfer") {
    // nothing to do
  } else if (inputXunit == "dSpacing") {
    // convert the x-units to Q/MomentumTransfer
    const double PI_2(2. * M_PI);
    std::for_each(inputQ.begin(), inputQ.end(), [&PI_2](double &Q) { Q /= PI_2; });
    std::transform(inputDQ.begin(), inputDQ.end(), inputQ.begin(), inputDQ.begin(), std::divides<double>());
    // reverse all of the arrays
    std::reverse(inputQ.begin(), inputQ.end());
    std::reverse(inputDQ.begin(), inputDQ.end());
    std::reverse(inputFOfQ.begin(), inputFOfQ.end());
    std::reverse(inputDfOfQ.begin(), inputDfOfQ.end());
  } else {
    std::stringstream msg;
    msg << "Input data x-axis with unit \"" << inputXunit
        << R"(" is not supported (use "MomentumTransfer" or "dSpacing"))";
    throw std::invalid_argument(msg.str());
  }
  g_log.debug() << "Input unit is " << inputXunit << "\n";

  // convert from histogram to density
  if (!inputWS->isHistogramData()) {
    g_log.warning() << "This algorithm has not been tested on density data "
                       "(only on histograms)\n";
    /* Don't do anything for now
    double deltaQ;
    for (size_t i = 0; i < inputFOfQ.size(); ++i)
    {
    deltaQ = inputQ[i+1] -inputQ[i];
    inputFOfQ[i] = inputFOfQ[i]*deltaQ;
    inputDfOfQ[i] = inputDfOfQ[i]*deltaQ; // TODO feels wrong
    inputQ[i] += -.5*deltaQ;
    inputDQ[i] += .5*(inputDQ[i] + inputDQ[i+1]); // TODO running average
    }
    inputQ.emplace_back(inputQ.back()+deltaQ);
    inputDQ.emplace_back(inputDQ.back()); // copy last value
    */
  }

  // convert to Q[S(Q)-1]
  string soqType = getProperty("InputSofQType");
  if (soqType == S_OF_Q) {
    g_log.information() << "Subtracting one from all values\n";
    // there is no error propagation for subtracting one
    std::for_each(inputFOfQ.begin(), inputFOfQ.end(), [](double &F) { F--; });
    soqType = S_OF_Q_MINUS_ONE;
  }
  if (soqType == S_OF_Q_MINUS_ONE) {
    g_log.information() << "Multiplying all values by Q\n";
    // error propagation
    for (size_t i = 0; i < inputDfOfQ.size(); ++i) {
      inputDfOfQ[i] = inputQ[i] * inputDfOfQ[i] + inputFOfQ[i] * inputDQ[i];
    }
    // convert the function
    std::transform(inputFOfQ.begin(), inputFOfQ.end(), inputQ.begin(), inputFOfQ.begin(), std::multiplies<double>());
    soqType = Q_S_OF_Q_MINUS_ONE;
  }
  if (soqType != Q_S_OF_Q_MINUS_ONE) {
    // should never get here
    std::stringstream msg;
    msg << "Do not understand InputSofQType = " << soqType;
    throw std::runtime_error(msg.str());
  }

  // determine Q-range
  size_t qmin_index = determineQminIndex(inputQ, inputFOfQ);
  size_t qmax_index = determineQmaxIndex(inputQ, inputFOfQ);
  g_log.notice() << "Adjusting to data: Qmin = " << inputQ[qmin_index] << " Qmax = " << inputQ[qmax_index] << "\n";

  // determine r axis for result
  const double rmax = getProperty("RMax");
  double rdelta = getProperty("DeltaR");
  if (isEmpty(rdelta))
    rdelta = M_PI / inputQ[qmax_index];
  auto sizer = static_cast<size_t>(rmax / rdelta);

  bool filter = getProperty("Filter");

  // create the output workspace
  API::MatrixWorkspace_sptr outputWS = create<Workspace2D>(1, Points(sizer));
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("AtomicDistance");
  outputWS->setYUnitLabel("PDF");

  outputWS->mutableRun().addProperty("Qmin", inputQ[qmin_index], "Angstroms^-1", true);
  outputWS->mutableRun().addProperty("Qmax", inputQ[qmax_index], "Angstroms^-1", true);

  BinEdges edges(sizer + 1, LinearGenerator(rdelta, rdelta));
  outputWS->setBinEdges(0, edges);

  auto &outputR = outputWS->mutableX(0);
  g_log.information() << "Using rmin = " << outputR.front() << "Angstroms and rmax = " << outputR.back()
                      << "Angstroms\n";
  // always calculate G(r) then convert
  auto &outputY = outputWS->mutableY(0);
  auto &outputE = outputWS->mutableE(0);

  // do the math
  for (size_t r_index = 0; r_index < sizer; r_index++) {
    const double r = outputR[r_index];
    double fs = 0;
    double error = 0;
    for (size_t q_index = qmin_index; q_index < qmax_index; q_index++) {
      const double q = inputQ[q_index];
      const double deltaq = inputQ[q_index] - inputQ[q_index - 1];
      double sinus = sin(q * r) * deltaq;

      // multiply by filter function sin(q*pi/qmax)/(q*pi/qmax)
      if (filter && q != 0) {
        const double lorchKernel = q * M_PI / inputQ[qmax_index];
        sinus *= sin(lorchKernel) / lorchKernel;
      }
      fs += sinus * inputFOfQ[q_index];
      error += (sinus * inputDfOfQ[q_index]) * (sinus * inputDfOfQ[q_index]);
      // g_log.debug() << "q[" << i << "] = " << q << "  dq = " << deltaq << "
      // S(q) =" << s;
      // g_log.debug() << "  d(gr) = " << temp << "  gr = " << gr << '\n';
    }

    // put the information into the output
    outputY[r_index] = fs * TWO_OVER_PI;
    outputE[r_index] = sqrt(error) * TWO_OVER_PI;
  }

  // convert to the correct form of PDF
  string pdfType = getProperty("PDFType");

  double rho0 = determineRho0();
  if (pdfType == LITTLE_G_OF_R || pdfType == RDF_OF_R)
    g_log.information() << "Using rho0 = " << rho0 << "\n";

  if (pdfType == BIG_G_OF_R) {
    // nothing to do
  } else if (pdfType == LITTLE_G_OF_R) {
    const double factor = 1. / (4. * M_PI * rho0);
    for (size_t i = 0; i < outputY.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      outputE[i] = outputE[i] / outputR[i];
      // transform the data
      outputY[i] = 1. + factor * outputY[i] / outputR[i];
    }
  } else if (pdfType == RDF_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < outputY.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      outputE[i] = outputE[i] * outputR[i];
      // transform the data
      outputY[i] = outputR[i] * outputY[i] + factor * outputR[i] * outputR[i];
    }
  } else {
    // should never get here
    std::stringstream msg;
    msg << "Do not understand PDFType = " << pdfType;
    throw std::runtime_error(msg.str());
  }

  // set property
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
