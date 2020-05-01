// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PDFFourierTransform2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>
#include <sstream>

namespace Mantid {
namespace Algorithms {

using std::string;
using namespace HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDFFourierTransform2)

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

const string FORWARD("Forward");

const string BACKWARD("Backward");

constexpr double TWO_OVER_PI(2. / M_PI);
} // namespace

const std::string PDFFourierTransform2::name() const {
  return "PDFFourierTransform";
}

int PDFFourierTransform2::version() const { return 2; }

const std::string PDFFourierTransform2::category() const {
  return "Diffraction\\Utility";
}

/** Initialize the algorithm's properties.
 */
void PDFFourierTransform2::init() {

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  S_OF_Q + ", " + S_OF_Q_MINUS_ONE + ", or " +
                      Q_S_OF_Q_MINUS_ONE);
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Result paired-distribution function");

  // Set up spectral dencity data type
  std::vector<std::string> inputTypes;
  inputTypes.emplace_back(S_OF_Q);
  inputTypes.emplace_back(S_OF_Q_MINUS_ONE);
  inputTypes.emplace_back(Q_S_OF_Q_MINUS_ONE);
  declareProperty("SofQType", S_OF_Q,
                  std::make_shared<StringListValidator>(inputTypes),
                  "To identify spectral density function");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty("DeltaQ", EMPTY_DBL(), mustBePositive,
                  "Step size of Q of S(Q) to calculate.  Default = "
                  ":math:`\\frac{\\pi}{R_{max}}`.");
  declareProperty(
      "Qmin", EMPTY_DBL(), mustBePositive,
      "Minimum Q in S(Q) to calculate in Fourier transform (optional).");
  declareProperty(
      "Qmax", EMPTY_DBL(), mustBePositive,
      "Maximum Q in S(Q) to calculate in Fourier transform. (optional)");
  declareProperty("Filter", false,
                  "Set to apply Lorch function filter to the input");

  // Set up PDF data type
  std::vector<std::string> outputTypes;
  outputTypes.emplace_back(BIG_G_OF_R);
  outputTypes.emplace_back(LITTLE_G_OF_R);
  outputTypes.emplace_back(RDF_OF_R);
  declareProperty("PDFType", BIG_G_OF_R,
                  std::make_shared<StringListValidator>(outputTypes),
                  "Type of output PDF including G(r)");

  declareProperty("DeltaR", EMPTY_DBL(), mustBePositive,
                  "Step size of r of G(r) to calculate.  Default = "
                  ":math:`\\frac{\\pi}{Q_{max}}`.");
  declareProperty("Rmin", EMPTY_DBL(), mustBePositive,
                  "Minimum r for G(r) to calculate.");
  declareProperty("Rmax", EMPTY_DBL(), mustBePositive,
                  "Maximum r for G(r) to calculate.");
  declareProperty(
      "rho0", EMPTY_DBL(), mustBePositive,
      "Average number density used for g(r) and RDF(r) conversions (optional)");

  string recipGroup("Reciprocal Space");
  setPropertyGroup("SofQType", recipGroup);
  setPropertyGroup("DeltaQ", recipGroup);
  setPropertyGroup("Qmin", recipGroup);
  setPropertyGroup("Qmax", recipGroup);
  setPropertyGroup("Filter", recipGroup);

  string realGroup("Real Space");
  setPropertyGroup("PDFType", realGroup);
  setPropertyGroup("DeltaR", realGroup);
  setPropertyGroup("Rmin", realGroup);
  setPropertyGroup("Rmax", realGroup);
  setPropertyGroup("rho0", realGroup);
}

std::map<string, string> PDFFourierTransform2::validateInputs() {
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
  const std::string inputXunit = inputWS->getAxis(0)->unit()->unitID();
  if (inputXunit != "MomentumTransfer" && inputXunit != "dSpacing" &&
      inputXunit != "AtomicDistance") {
    result["InputWorkspace"] = "Input workspace units not supported";
  }

  return result;
}

size_t PDFFourierTransform2::determineMinIndex(double min,
                                               const std::vector<double> &X,
                                               const std::vector<double> &Y) {
  // check against available X-range
  if (isEmpty(min)) {
    min = X.front();
  } else if (min < X.front()) {
    g_log.information(
        "Specified input min < range of data. Adjusting to data range.");
    min = X.front();
  }

  // get index for the min from the X-range
  auto iter = std::upper_bound(X.begin(), X.end(), min);
  size_t min_index = std::distance(X.begin(), iter);
  if (min_index == 0)
    min_index += 1; // so there doesn't have to be a check in integration loop

  // go to first non-nan value
  iter = std::find_if(std::next(Y.begin(), min_index), Y.end(),
                      static_cast<bool (*)(double)>(std::isnormal));
  size_t first_normal_index = std::distance(Y.begin(), iter);
  if (first_normal_index > min_index) {
    g_log.information(
        "Specified input min where data is nan/inf. Adjusting to data range.");
    min_index = first_normal_index;
  }

  return min_index;
}

size_t PDFFourierTransform2::determineMaxIndex(double max,
                                               const std::vector<double> &X,
                                               const std::vector<double> &Y) {
  // check against available X-range
  if (isEmpty(max)) {
    max = X.back();
  } else if (max > X.back()) {
    g_log.information()
        << "Specified input max > range of data. Adjusting to data range.\n";
    max = X.back();
  }

  // get pointers for the data range
  auto iter = std::lower_bound(X.begin(), X.end(), max);
  size_t max_index = std::distance(X.begin(), iter);

  // go to first non-nan value
  auto back_iter = std::find_if(Y.rbegin(), Y.rend(),
                                static_cast<bool (*)(double)>(std::isnormal));
  size_t first_normal_index =
      Y.size() - std::distance(Y.rbegin(), back_iter) - 1;
  if (first_normal_index < max_index) {
    g_log.information(
        "Specified input max where data is nan/inf. Adjusting to data range.");
    max_index = first_normal_index;
  }

  return max_index;
}

double PDFFourierTransform2::determineRho0() {
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

void PDFFourierTransform2::convertToSQMinus1(std::vector<double> &FOfQ,
                                             std::vector<double> &Q,
                                             std::vector<double> &DFOfQ,
                                             std::vector<double> &DQ) {
  // convert to Q[S(Q)-1]
  string soqType = getProperty("SofQType");
  if (soqType == S_OF_Q) {
    g_log.information() << "Subtracting one from all values\n";
    // there is no error propagation for subtracting one
    std::for_each(FOfQ.begin(), FOfQ.end(), [](double &F) { F--; });
    soqType = S_OF_Q_MINUS_ONE;
  }
  if (soqType == Q_S_OF_Q_MINUS_ONE) {
    g_log.information() << "Dividing all values by Q\n";
    // error propagation
    for (size_t i = 0; i < DFOfQ.size(); ++i) {
      DFOfQ[i] = (Q[i] / DQ[i] + FOfQ[i] / DFOfQ[i]) * (FOfQ[i] / Q[i]);
    }
    // convert the function
    std::transform(FOfQ.begin(), FOfQ.end(), FOfQ.begin(), Q.begin(),
                   std::divides<double>());
    soqType = S_OF_Q_MINUS_ONE;
  }
  if (soqType != S_OF_Q_MINUS_ONE) {
    // should never get here
    std::stringstream msg;
    msg << "Do not understand SofQType = " << soqType;
    throw std::runtime_error(msg.str());
  }
  return;
}

void PDFFourierTransform2::convertToLittleGRPlus1(std::vector<double> &FOfR,
                                                  std::vector<double> &R,
                                                  std::vector<double> &DFOfR,
                                                  std::vector<double> &DR) {
  string PDFType = getProperty("PDFType");
  double rho0 = determineRho0();
  if (PDFType == LITTLE_G_OF_R) {
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // transform the data
      FOfR[i] = FOfR[i] + 1;
    }
  } else if (PDFType == BIG_G_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = (R[i] / DR[i] + FOfR[i] / DFOfR[i]) * (FOfR[i] / R[i]);
      // transform the data
      FOfR[i] = FOfR[i] / factor / R[i];
    }
  } else if (PDFType == RDF_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = (2.0 * R[i] / DR[i] + FOfR[i] / DFOfR[i]) * (FOfR[i] / R[i]);
      // transform the data
      FOfR[i] = FOfR[i] / factor / R[i] / R[i];
    }
  }
  return;
}

void PDFFourierTransform2::convertFromSQMinus1(
    HistogramData::HistogramY &FOfQ, HistogramData::HistogramX &Q,
    HistogramData::HistogramE &DFOfQ) {
  // convert to S(Q)-1
  string outputType = getProperty("SofQType");
  if (outputType == S_OF_Q) {
    for (size_t i = 0; i < FOfQ.size(); ++i) {
      // transform the data
      FOfQ[i] = FOfQ[i] + 1.0;
    }
  } else if (outputType == Q_S_OF_Q_MINUS_ONE) {
    for (size_t i = 0; i < FOfQ.size(); ++i) {
      DFOfQ[i] = Q[i] * DFOfQ[i];
      FOfQ[i] = FOfQ[i] * Q[i];
    }
  }
  return;
}

void PDFFourierTransform2::convertFromLittleGRPlus1(
    HistogramData::HistogramY &FOfR, HistogramData::HistogramX &R,
    HistogramData::HistogramE &DFOfR) {
  // convert to the correct form of PDF
  double rho0 = determineRho0();
  string outputType = getProperty("PDFType");
  if (outputType == LITTLE_G_OF_R) {
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // transform the data
      FOfR[i] = FOfR[i] - 1;
    }
  } else if (outputType == BIG_G_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = DFOfR[i] * R[i];
      // transform the data
      FOfR[i] = factor * (FOfR[i]) * R[i];
    }
  } else if (outputType == RDF_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = DFOfR[i] * R[i];
      // transform the data
      FOfR[i] = (FOfR[i] + 1.0) * factor * R[i] * R[i];
    }
  }
}
//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PDFFourierTransform2::exec() {
  // get input data
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  auto inputX = inputWS->x(0).rawData();           //  x for input
  std::vector<double> inputDX(inputX.size(), 0.0); // dx for input
  if (inputWS->sharedDx(0))
    inputDX = inputWS->dx(0).rawData();
  auto inputY = inputWS->y(0).rawData();  //  y for input
  auto inputDY = inputWS->e(0).rawData(); // dy for input

  // transform input data into Q/MomentumTransfer
  string direction = FORWARD;
  const std::string inputXunit = inputWS->getAxis(0)->unit()->unitID();
  if (inputXunit == "MomentumTransfer") {
    // nothing to do
    direction = FORWARD;
  } else if (inputXunit == "dSpacing") {
    // convert the x-units to Q/MomentumTransfer
    const double PI_2(2. * M_PI);
    std::for_each(inputX.begin(), inputX.end(),
                  [&PI_2](double &Q) { Q /= PI_2; });
    std::transform(inputDX.begin(), inputDX.end(), inputX.begin(),
                   inputDX.begin(), std::divides<double>());
    // reverse all of the arrays
    std::reverse(inputX.begin(), inputX.end());
    std::reverse(inputDX.begin(), inputDX.end());
    std::reverse(inputY.begin(), inputY.end());
    std::reverse(inputDY.begin(), inputDY.end());
    direction = FORWARD;
  } else if (inputXunit == "AtomicDistance") {
    direction = BACKWARD;
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

  // convert to S(Q)-1 or g(R)+1
  double inDelta, inMin, inMax, outDelta, outMin, outMax;
  if (direction == FORWARD) {
    convertToSQMinus1(inputY, inputX, inputDY, inputDX);
    inDelta = getProperty("DeltaQ");
    inMin = getProperty("Qmin");
    inMax = getProperty("Qmax");
    outDelta = getProperty("DeltaR");
    outMin = getProperty("Rmin");
    if (isEmpty(outMin)) {
      outMin = 0;
    }
    outMax = getProperty("Rmax");
    if (isEmpty(outMax)) {
      outMax = 20;
    }
  } else if (direction == BACKWARD) {
    convertToLittleGRPlus1(inputY, inputX, inputDY, inputDX);
    inDelta = getProperty("DeltaR");
    inMin = getProperty("Rmin");
    inMax = getProperty("Rmax");
    outDelta = getProperty("DeltaQ");
    outMin = getProperty("Qmin");
    if (isEmpty(outMin)) {
      outMin = 0;
    }
    outMax = getProperty("Qmax");
    if (isEmpty(outMax)) {
      outMax = 40;
    }
  }

  // determine input-range
  size_t Xmin_index = determineMinIndex(inMin, inputX, inputY);
  size_t Xmax_index = determineMaxIndex(inMax, inputX, inputY);
  g_log.notice() << "Adjusting to data: input min = " << inputX[Xmin_index]
                 << " input max = " << inputX[Xmax_index] << "\n";
  // determine r axis for result
  if (isEmpty(outDelta))
    outDelta = M_PI / inputX[Xmax_index];
  auto sizer = static_cast<size_t>(outMax / outDelta);

  bool filter = getProperty("Filter");

  // create the output workspace

  API::MatrixWorkspace_sptr outputWS = create<Workspace2D>(1, Points(sizer));
  if (direction == FORWARD) {
    outputWS->getAxis(0)->unit() =
        UnitFactory::Instance().create("AtomicDistance");
    outputWS->setYUnitLabel("PDF");
    outputWS->mutableRun().addProperty("Qmin", inputX[Xmin_index],
                                       "Angstroms^-1", true);
    outputWS->mutableRun().addProperty("Qmax", inputX[Xmax_index],
                                       "Angstroms^-1", true);
  } else if (direction == BACKWARD) {
    outputWS->getAxis(0)->unit() =
        UnitFactory::Instance().create("MomentumTransfer");
    outputWS->setYUnitLabel("Spectrum Dencity");
    outputWS->mutableRun().addProperty("Rmin", inputX[Xmin_index], "Angstroms",
                                       true);
    outputWS->mutableRun().addProperty("Rmax", inputX[Xmax_index], "Angstroms",
                                       true);
  }
  outputWS->setDistribution(TRUE);
  BinEdges edges(sizer + 1, LinearGenerator(outDelta, outDelta));
  outputWS->setBinEdges(0, edges);
  auto &outputX = outputWS->mutableX(0);
  g_log.information() << "Using output min = " << outputX.front()
                      << "and output max = " << outputX.back() << "\n";
  // always calculate G(r) then convert
  auto &outputY = outputWS->mutableY(0);
  auto &outputE = outputWS->mutableE(0);

  // do the math

  double rho0 = determineRho0();
  double corr;
  for (size_t r_index = 0; r_index < sizer; r_index++) {
    const double r = outputX[r_index];
    if (direction == FORWARD) {
      corr = 0.5 / M_PI / M_PI / rho0;
    } else if (direction == BACKWARD) {
      corr = 4.0 * M_PI * rho0;
    }
    const double rfac = corr / (r * r * r);

    double fs = 0;
    double error = 0;
    for (size_t x_index = Xmin_index; x_index < Xmax_index; x_index++) {
      const double x1 = inputX[x_index];
      const double x2 = inputX[x_index + 1];
      const double sinx1 = sin(x1 * r) - x1 * r * cos(x1 * r);
      const double sinx2 = sin(x2 * r) - x2 * r * cos(x2 * r);
      double sinus = sinx2 - sinx1;

      // multiply by filter function sin(q*pi/qmax)/(q*pi/qmax)
      if (filter && x1 != 0) {
        const double lorchKernel = x1 * M_PI / inputX[Xmax_index];
        sinus *= sin(lorchKernel) / lorchKernel;
      }

      fs += sinus * inputY[x_index];

      error += (sinus * inputDY[x_index]) * (sinus * inputDY[x_index]);
    }

    // put the information into the output
    outputY[r_index] = fs * rfac;
    outputE[r_index] = sqrt(error) * rfac;
  }

  if (direction == FORWARD) {
    convertFromLittleGRPlus1(outputY, outputX, outputE);
  } else if (direction == BACKWARD) {
    convertFromSQMinus1(outputY, outputX, outputE);
  }

  // set property
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid