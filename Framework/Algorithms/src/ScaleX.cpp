// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ScaleX.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ScaleX)

/**
 * Default constructor
 */
ScaleX::ScaleX()
    : API::Algorithm(), m_progress(nullptr), m_algFactor(1.0), m_parname(), m_combine(false), m_binOp(), m_wi_min(-1),
      m_wi_max(-1) {}

/**
 * Initialisation method. Declares properties to be used in algorithm.
 */
void ScaleX::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
  auto isDouble = std::make_shared<BoundedValidator<double>>();
  declareProperty("Factor", m_algFactor, isDouble,
                  "The value by which to scale the X-axis of the input "
                  "workspace. Default is 1.0");
  std::vector<std::string> op(2);
  op[0] = "Multiply";
  op[1] = "Add";
  declareProperty("Operation", "Multiply", std::make_shared<StringListValidator>(op),
                  "Whether to multiply by, or add factor");
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("IndexMin", 0, mustBePositive,
                  "The workspace index of the first spectrum to scale. Only "
                  "used if IndexMax is set.");
  declareProperty("IndexMax", Mantid::EMPTY_INT(), mustBePositive,
                  "The workspace index of the last spectrum to scale. Only "
                  "used if explicitly set.");
  // Add InstrumentParameter property here so as not to mess with the parameter
  // order for current scripts
  declareProperty("InstrumentParameter", m_parname,
                  "The name of an instrument parameter whose value is used to "
                  "scale as the input factor");
  declareProperty("Combine", m_combine,
                  "If true, combine the value given in the Factor property with the value "
                  "obtained from the instrument parameter. The factors are combined using "
                  "the operation specified "
                  "in the Operation parameter");
}

/**
 * Executes the algorithm
 */
void ScaleX::exec() {
  // Get input workspace and offset
  const MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
  m_algFactor = getProperty("Factor");
  m_parname = getPropertyValue("InstrumentParameter");
  m_combine = getProperty("Combine");
  if (m_combine && m_parname.empty()) {
    throw std::invalid_argument("Combine behaviour requested but the "
                                "InstrumentParameter argument is blank.");
  }

  const std::string op = getPropertyValue("Operation");
  API::MatrixWorkspace_sptr outputW = createOutputWS(inputW);
  // Get number of histograms
  auto histnumber = static_cast<int>(inputW->getNumberHistograms());
  m_progress = std::make_unique<API::Progress>(this, 0.0, 1.0, histnumber + 1);
  m_progress->report("Scaling X");
  m_wi_min = 0;
  m_wi_max = histnumber - 1;
  // check if workspace indexes have been set
  int tempwi_min = getProperty("IndexMin");
  int tempwi_max = getProperty("IndexMax");
  if (tempwi_max != Mantid::EMPTY_INT()) {
    if ((m_wi_min <= tempwi_min) && (tempwi_min <= tempwi_max) && (tempwi_max <= m_wi_max)) {
      m_wi_min = tempwi_min;
      m_wi_max = tempwi_max;
    } else {
      g_log.error("Invalid Workspace Index min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }
  // Setup appropriate binary function
  const bool multiply = (op == "Multiply");
  if (multiply)
    m_binOp = std::multiplies<double>();
  else
    m_binOp = std::plus<double>();

  // Check if its an event workspace
  EventWorkspace_const_sptr eventWS = std::dynamic_pointer_cast<const EventWorkspace>(inputW);
  if (eventWS != nullptr) {
    this->execEvent();
    return;
  }
  const auto &spectrumInfo = inputW->spectrumInfo();

  // do the shift in X
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputW, *outputW))
  for (int i = 0; i < histnumber; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // Copy y and e data
    outputW->setHistogram(i, inputW->histogram(i));

    auto &outX = outputW->mutableX(i);
    auto &outY = outputW->mutableY(i);
    auto &outE = outputW->mutableE(i);

    const auto &inX = inputW->x(i);
    // Change bin value by offset
    if ((i >= m_wi_min) && (i <= m_wi_max)) {
      double factor = getScaleFactor(inputW, spectrumInfo, i);
      // Do the offsetting
      using std::placeholders::_1;
      std::transform(inX.begin(), inX.end(), outX.begin(), std::bind(m_binOp, _1, factor));
      // reverse the vector if multiplicative factor was negative
      if (multiply && factor < 0.0) {
        std::reverse(outX.begin(), outX.end());
        std::reverse(outY.begin(), outY.end());
        std::reverse(outE.begin(), outE.end());
      }
    }

    m_progress->report("Scaling X");

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Copy units
  if (outputW->getAxis(0)->unit().get())
    outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
  try {
    if (inputW->getAxis(1)->unit().get())
      outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
  } catch (Exception::IndexError &) {
    // OK, so this isn't a Workspace2D
  }
  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputW);
}

void ScaleX::execEvent() {
  g_log.information("Processing event workspace");
  const MatrixWorkspace_const_sptr matrixInputWS = this->getProperty("InputWorkspace");
  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  if (matrixOutputWS != matrixInputWS) {
    matrixOutputWS = matrixInputWS->clone();
    setProperty("OutputWorkspace", matrixOutputWS);
  }
  auto outputWS = std::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  const auto &spectrumInfo = matrixInputWS->spectrumInfo();

  const std::string op = getPropertyValue("Operation");
  auto numHistograms = static_cast<int>(outputWS->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int i = 0; i < numHistograms; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    // Do the offsetting
    if ((i >= m_wi_min) && (i <= m_wi_max)) {
      auto factor = getScaleFactor(outputWS, spectrumInfo, i);
      if (op == "Multiply") {
        outputWS->getSpectrum(i).scaleTof(factor);
        if (factor < 0) {
          outputWS->getSpectrum(i).reverse();
        }
      } else if (op == "Add") {
        outputWS->getSpectrum(i).addTof(factor);
      }
    }
    m_progress->report("Scaling X");
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  outputWS->clearMRU();
}

API::MatrixWorkspace_sptr ScaleX::createOutputWS(const API::MatrixWorkspace_sptr &input) {
  // Check whether input = output to see whether a new workspace is required.
  MatrixWorkspace_sptr output = getProperty("OutputWorkspace");
  if (input != output) {
    // Create new workspace for output from old
    output = API::WorkspaceFactory::Instance().create(input);
  }
  return output;
}

/**
 * If the InstrumentParameter property is set then it attempts to retrieve the
 * parameter
 * from the component, else it returns the value of the Factor property
 * @param inputWS A pointer to the input workspace
 * @param spectrumInfo The SpectrumInfo for the input workspace
 * @param index The current index to inspect
 * @return Value for the scale factor
 */
double ScaleX::getScaleFactor(const API::MatrixWorkspace_const_sptr &inputWS,
                              const Mantid::API::SpectrumInfo &spectrumInfo, const size_t index) {
  if (m_parname.empty())
    return m_algFactor;

  // Try and get factor from component. If we see a DetectorGroup this will
  // use the first component
  if (!spectrumInfo.hasDetectors(index)) {
    return 0.0;
  }

  const auto &detectorInfo = inputWS->detectorInfo();
  const auto detIndex = spectrumInfo.spectrumDefinition(index)[0].first;
  const auto &det = detectorInfo.detector(detIndex);
  const auto &pmap = inputWS->constInstrumentParameters();
  auto par = pmap.getRecursive(det.getComponentID(), m_parname);
  if (par) {
    if (!m_combine)
      return par->value<double>();
    else
      return m_binOp(m_algFactor, par->value<double>());
  } else {
    std::ostringstream os;
    os << "Spectrum at index '" << index << "' has no parameter named '" << m_parname << "'\n";
    throw std::runtime_error(os.str());
  }
}

} // namespace Mantid::Algorithms
