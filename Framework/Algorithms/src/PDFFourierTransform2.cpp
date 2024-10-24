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
#include "MantidKernel/InvisibleProperty.h"
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
/// Total radial distribution function
const string G_K_OF_R("G_k(r)");

/// Normalized intensity
const string S_OF_Q("S(Q)");
/// Asymptotes to zero
const string S_OF_Q_MINUS_ONE("S(Q)-1");
/// Kernel of the Fourier transform
const string Q_S_OF_Q_MINUS_ONE("Q[S(Q)-1]");

const string FORWARD("Forward");

const string BACKWARD("Backward");

} // namespace

const std::string PDFFourierTransform2::name() const { return "PDFFourierTransform"; }

int PDFFourierTransform2::version() const { return 2; }

const std::string PDFFourierTransform2::category() const { return "Diffraction\\Utility"; }

/** Initialize the algorithm's properties.
 */
void PDFFourierTransform2::init() {
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "Input spectrum density or paired-distribution function");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Result paired-distribution function or Input spectrum density");

  std::vector<std::string> directionOptions;
  directionOptions.emplace_back(FORWARD);
  directionOptions.emplace_back(BACKWARD);
  declareProperty("Direction", FORWARD, std::make_shared<StringListValidator>(directionOptions),
                  "The direction of the fourier transform");
  declareProperty("rho0", EMPTY_DBL(), mustBePositive,
                  "Average number density used for g(r) and RDF(r) conversions (optional)");
  declareProperty("Filter", false, "Set to apply Lorch function filter to the input");

  // Set up spectral density data type
  std::vector<std::string> soqTypes;
  soqTypes.emplace_back(S_OF_Q);
  soqTypes.emplace_back(S_OF_Q_MINUS_ONE);
  soqTypes.emplace_back(Q_S_OF_Q_MINUS_ONE);
  declareProperty("InputSofQType", S_OF_Q, std::make_shared<StringListValidator>(soqTypes),
                  "To identify spectral density function (deprecated)");
  setPropertySettings("InputSofQType", std::make_unique<InvisibleProperty>());
  declareProperty("SofQType", S_OF_Q, std::make_shared<StringListValidator>(soqTypes),
                  "To identify spectral density function");
  mustBePositive->setLower(0.0);

  declareProperty("DeltaQ", EMPTY_DBL(), mustBePositive,
                  "Step size of Q of S(Q) to calculate.  Default = "
                  ":math:`\\frac{\\pi}{R_{max}}`.");
  setPropertySettings("DeltaQ", std::make_unique<EnabledWhenProperty>("Direction", IS_EQUAL_TO, BACKWARD));
  declareProperty("Qmin", EMPTY_DBL(), mustBePositive,
                  "Minimum Q in S(Q) to calculate in Fourier transform (optional).");
  setPropertySettings("Qmin", std::make_unique<EnabledWhenProperty>("Direction", IS_EQUAL_TO, FORWARD));
  declareProperty("Qmax", EMPTY_DBL(), mustBePositive,
                  "Maximum Q in S(Q) to calculate in Fourier transform. "
                  "(optional, defaults to 40 on backward transform.)");

  // Set up PDF data type
  std::vector<std::string> pdfTypes;
  pdfTypes.emplace_back(BIG_G_OF_R);
  pdfTypes.emplace_back(LITTLE_G_OF_R);
  pdfTypes.emplace_back(RDF_OF_R);
  pdfTypes.emplace_back(G_K_OF_R);
  declareProperty("PDFType", BIG_G_OF_R, std::make_shared<StringListValidator>(pdfTypes),
                  "Type of output PDF including G(r)");

  declareProperty("DeltaR", EMPTY_DBL(), mustBePositive,
                  "Step size of r of G(r) to calculate.  Default = "
                  ":math:`\\frac{\\pi}{Q_{max}}`.");
  setPropertySettings("DeltaR", std::make_unique<EnabledWhenProperty>("Direction", IS_EQUAL_TO, FORWARD));
  declareProperty("Rmin", EMPTY_DBL(), mustBePositive, "Minimum r for G(r) to calculate. (optional)");
  setPropertySettings("Rmin", std::make_unique<EnabledWhenProperty>("Direction", IS_EQUAL_TO, BACKWARD));
  declareProperty("Rmax", EMPTY_DBL(), mustBePositive,
                  "Maximum r for G(r) to calculate. (optional, defaults to 20 "
                  "on forward transform.)");

  string recipGroup("Reciprocal Space");
  setPropertyGroup("SofQType", recipGroup);
  setPropertyGroup("DeltaQ", recipGroup);
  setPropertyGroup("Qmin", recipGroup);
  setPropertyGroup("Qmax", recipGroup);

  string realGroup("Real Space");
  setPropertyGroup("PDFType", realGroup);
  setPropertyGroup("DeltaR", realGroup);
  setPropertyGroup("Rmin", realGroup);
  setPropertyGroup("Rmax", realGroup);
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
  if (inputXunit != "MomentumTransfer" && inputXunit != "dSpacing" && inputXunit != "AtomicDistance") {
    result["InputWorkspace"] = "Input workspace units not supported";
  }

  return result;
}

size_t PDFFourierTransform2::determineMinIndex(double min, const std::vector<double> &X, const std::vector<double> &Y) {
  // check against available X-range
  if (isEmpty(min)) {
    min = X.front();
  } else if (min < X.front()) {
    g_log.information("Specified input min < range of data. Adjusting to data range.");
    min = X.front();
  }

  // get index for the min from the X-range, tightening the range if a partial x bin value is provided
  auto iter = std::lower_bound(X.begin(), X.end(), min);
  size_t min_index = std::distance(X.begin(), iter);

  // go to first non-nan value
  auto iter_first_normal =
      std::find_if(std::next(Y.begin(), min_index), Y.end(), static_cast<bool (*)(double)>(std::isnormal));
  size_t first_normal_index = std::distance(Y.begin(), iter_first_normal);
  if (first_normal_index > min_index) {
    g_log.information("Specified input min where data is nan/inf or zero. Adjusting to data range.");
    min_index = first_normal_index;
  }

  return min_index;
}

size_t PDFFourierTransform2::determineMaxIndex(double max, const std::vector<double> &X, const std::vector<double> &Y) {
  // check against available X-range
  if (isEmpty(max)) {
    max = X.back();
  } else if (max > X.back()) {
    g_log.information() << "Specified input max > range of data. Adjusting to data range.\n";
    max = X.back();
  }

  // get index for the max from the X-range, tightening the range if a partial x bin value is provided
  auto iter = std::upper_bound(X.begin(), X.end(), max) - 1;
  size_t max_index = std::distance(X.begin(), iter);

  // go to first non-nan value. This works for both histogram (bin edge) data and
  // point data, as the integration proceeds by calculating rectangles between pairs of X values.
  // For point data the Y value corresponding to the left of this pair is used in the nested for loop in the exec.
  auto back_iter = std::find_if(Y.rbegin(), Y.rend(), static_cast<bool (*)(double)>(std::isnormal));
  size_t first_normal_index = Y.size() - std::distance(Y.rbegin(), back_iter);
  if (first_normal_index < max_index) {
    g_log.information("Specified input max where data is nan/inf or zero. Adjusting to data range.");
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

void PDFFourierTransform2::convertToSQMinus1(std::vector<double> &FOfQ, std::vector<double> &Q,
                                             std::vector<double> &DFOfQ, const std::vector<double> &DQ) {
  // convert to S(Q)-1
  string soqType = getProperty("SofQType");
  string inputSOQType = getProperty("InputSofQType");
  if (!isDefault("InputSofQType") && isDefault("SofQType")) {
    soqType = inputSOQType;
    g_log.warning() << "InputSofQType has been deprecated and replaced by SofQType\n";
  }
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
    std::transform(FOfQ.begin(), FOfQ.end(), FOfQ.begin(), Q.begin(), std::divides<double>());
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

void PDFFourierTransform2::convertToLittleGRMinus1(std::vector<double> &FOfR, const std::vector<double> &R,
                                                   std::vector<double> &DFOfR, const std::vector<double> &DR,
                                                   const std::string &PDFType, const double &rho0,
                                                   const double &cohScatLen) {
  if (PDFType == LITTLE_G_OF_R) {
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // transform the data
      FOfR[i] = FOfR[i] - 1.0;
    }
  } else if (PDFType == BIG_G_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = (R[i] / DR[i] + FOfR[i] / DFOfR[i]) * (FOfR[i] / R[i]);
      // transform the data
      FOfR[i] = FOfR[i] / (factor * R[i]);
    }
  } else if (PDFType == RDF_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = (2.0 * R[i] / DR[i] + FOfR[i] / DFOfR[i]) * (FOfR[i] / R[i]);
      // transform the data
      FOfR[i] = FOfR[i] / (factor * R[i] * R[i]) - 1.0;
    }
  } else if (PDFType == G_K_OF_R) {
    const double factor = 0.01 * pow(cohScatLen, 2);

    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = DFOfR[i] / factor;
      // transform the data
      FOfR[i] = FOfR[i] / factor;
    }
  }
  return;
}

void PDFFourierTransform2::convertFromSQMinus1(HistogramData::HistogramY &FOfQ, const HistogramData::HistogramX &Q,
                                               HistogramData::HistogramE &DFOfQ) {
  // convert to S(Q)-1string
  string soqType = getProperty("SofQType");
  string inputSOQType = getProperty("InputSofQType");
  if (!isDefault("InputSofQType") && isDefault("SofQType")) {
    soqType = inputSOQType;
    g_log.warning() << "InputSofQType has been deprecated and replaced by SofQType\n";
  }
  if (soqType == S_OF_Q) {
    for (size_t i = 0; i < FOfQ.size(); ++i) {
      // transform the data
      FOfQ[i] = FOfQ[i] + 1.0;
    }
  } else if (soqType == Q_S_OF_Q_MINUS_ONE) {
    for (size_t i = 0; i < FOfQ.size(); ++i) {
      DFOfQ[i] = Q[i] * DFOfQ[i];
      FOfQ[i] = FOfQ[i] * Q[i];
    }
  }
  return;
}

void PDFFourierTransform2::convertFromLittleGRMinus1(HistogramData::HistogramY &FOfR,
                                                     const HistogramData::HistogramX &R,
                                                     HistogramData::HistogramE &DFOfR, const std::string &PDFType,
                                                     const double &rho0, const double &cohScatLen) {
  // convert to the correct form of PDF
  if (PDFType == LITTLE_G_OF_R) {
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // transform the data
      FOfR[i] = FOfR[i] + 1.0;
    }
  } else if (PDFType == BIG_G_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = DFOfR[i] * R[i];
      // transform the data
      FOfR[i] = FOfR[i] * factor * R[i];
    }
  } else if (PDFType == RDF_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = DFOfR[i] * R[i];
      // transform the data
      FOfR[i] = (FOfR[i] + 1.0) * factor * R[i] * R[i];
    }
  } else if (PDFType == G_K_OF_R) {
    const double factor = 0.01 * pow(cohScatLen, 2);

    for (size_t i = 0; i < FOfR.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      DFOfR[i] = DFOfR[i] * factor;
      // transform the data
      FOfR[i] = FOfR[i] * factor;
    }
  }
  return;
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
  string direction = getProperty("Direction");
  const std::string inputXunit = inputWS->getAxis(0)->unit()->unitID();
  if (inputXunit == "MomentumTransfer") {
    // nothing to do
  } else if (inputXunit == "dSpacing") {
    // convert the x-units to Q/MomentumTransfer
    const double PI_2(2. * M_PI);
    std::for_each(inputX.begin(), inputX.end(), [&PI_2](double &Q) { Q /= PI_2; });
    std::transform(inputDX.begin(), inputDX.end(), inputX.begin(), inputDX.begin(), std::divides<double>());
    // reverse all of the arrays
    std::reverse(inputX.begin(), inputX.end());
    std::reverse(inputDX.begin(), inputDX.end());
    std::reverse(inputY.begin(), inputY.end());
    std::reverse(inputDY.begin(), inputDY.end());
  } else if (inputXunit == "AtomicDistance") {
    // nothing to do
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

  const std::string PDFType = getProperty("PDFType");
  double rho0 = determineRho0();
  const Kernel::Material &material = inputWS->sample().getMaterial();
  const auto cohScatLen = material.cohScatterLength();

  // A material needs to be provided in order to calculate G_K(r)
  if (PDFType == "G_k(r)" && cohScatLen == 0.0) {
    std::stringstream msg;
    msg << "Coherent Scattering Length is zero. Please check a sample material has been specified";
    throw std::runtime_error(msg.str());
  }

  // convert to S(Q)-1 or g(R)+1
  if (direction == FORWARD) {
    convertToSQMinus1(inputY, inputX, inputDY, inputDX);
  } else if (direction == BACKWARD) {
    convertToLittleGRMinus1(inputY, inputX, inputDY, inputDX, PDFType, rho0, cohScatLen);
  }

  double inMin, inMax, outDelta, outMax;
  inMin = getProperty("Qmin");
  inMax = getProperty("Qmax");
  outDelta = getProperty("DeltaR");
  outMax = getProperty("Rmax");
  if (isEmpty(outMax)) {
    outMax = 20;
  }
  if (direction == BACKWARD) {
    inMin = getProperty("Rmin");
    inMax = getProperty("Rmax");
    outDelta = getProperty("DeltaQ");
    outMax = getProperty("Qmax");
    if (isEmpty(outMax)) {
      outMax = 40;
    }
  }

  // determine input-range
  size_t Xmin_index = determineMinIndex(inMin, inputX, inputY);
  size_t Xmax_index = determineMaxIndex(inMax, inputX, inputY);
  g_log.notice() << "Adjusting to data: input min = " << inputX[Xmin_index] << " input max = " << inputX[Xmax_index]
                 << "\n";
  // determine r axis for result
  if (isEmpty(outDelta))
    outDelta = M_PI / inputX[Xmax_index];
  auto sizer = static_cast<size_t>(outMax / outDelta);

  bool filter = getProperty("Filter");

  // create the output workspace

  API::MatrixWorkspace_sptr outputWS = create<Workspace2D>(1, Points(sizer));
  outputWS->copyExperimentInfoFrom(inputWS.get());
  if (direction == FORWARD) {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("AtomicDistance");
    outputWS->setYUnitLabel("PDF");
    outputWS->mutableRun().addProperty("Qmin", inputX[Xmin_index], "Angstroms^-1", true);
    outputWS->mutableRun().addProperty("Qmax", inputX[Xmax_index], "Angstroms^-1", true);
  } else if (direction == BACKWARD) {
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    outputWS->setYUnitLabel("Spectrum Density");
    outputWS->mutableRun().addProperty("Rmin", inputX[Xmin_index], "Angstroms", true);
    outputWS->mutableRun().addProperty("Rmax", inputX[Xmax_index], "Angstroms", true);
  }
  outputWS->setDistribution(true);
  BinEdges edges(sizer + 1, LinearGenerator(outDelta, outDelta));
  outputWS->setBinEdges(0, edges);
  auto &outputX = outputWS->mutableX(0);
  g_log.information() << "Using output min = " << outputX.front() << "and output max = " << outputX.back() << "\n";
  // always calculate G(r) then convert
  auto &outputY = outputWS->mutableY(0);
  auto &outputE = outputWS->mutableE(0);

  // do the math
  // Evaluate integral of Qsin(QR) over the Q bin width rather than just take value at bin centre
  // Useful if Q bins widths are large - width typically increases with Q for TOF data. Following Gudrun approach

  double corr = 0.5 / M_PI / M_PI / rho0;
  if (direction == BACKWARD) {
    corr = 4.0 * M_PI * rho0;
  }
  for (size_t outXIndex = 0; outXIndex < sizer; outXIndex++) {
    const double outX = outputX[outXIndex];
    const double outXFac = corr / (outX * outX * outX);

    double fs = 0;
    double errorSquared = 0;
    auto pdfIntegral = [outX](const double x) -> double { return sin(x * outX) - x * outX * cos(x * outX); };
    double inX2 = inputX[Xmin_index];
    double integralX2 = pdfIntegral(inX2);
    const double inXMax = inputX[Xmax_index];

    for (size_t inXIndex = Xmin_index; inXIndex < Xmax_index; inXIndex++) {
      const double inX1 = inX2;
      inX2 = inputX[inXIndex + 1];
      const double integralX1 = integralX2;
      integralX2 = pdfIntegral(inX2);
      double defIntegral = integralX2 - integralX1;

      // multiply by filter function sin(q*pi/qmax)/(q*pi/qmax)
      if (filter && inX1 != 0) {
        const double lorchKernel = inX1 * M_PI / inXMax;
        defIntegral *= sin(lorchKernel) / lorchKernel;
      }
      fs += defIntegral * inputY[inXIndex];

      const double error = defIntegral * inputDY[inXIndex];
      errorSquared += error * error;
    }

    // put the information into the output
    outputY[outXIndex] = fs * outXFac;
    outputE[outXIndex] = sqrt(errorSquared) * outXFac;
  }

  if (direction == FORWARD) {
    convertFromLittleGRMinus1(outputY, outputX, outputE, PDFType, rho0, cohScatLen);
  } else if (direction == BACKWARD) {
    convertFromSQMinus1(outputY, outputX, outputE);
  }

  // set property
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
