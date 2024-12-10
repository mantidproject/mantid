// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/XDataConverter.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/RebinnedOutput.h"

namespace Mantid::Algorithms {

using API::MatrixWorkspace_const_sptr;
using API::MatrixWorkspace_sptr;
using API::Progress;
using API::WorkspaceFactory;
using API::WorkspaceProperty;
using DataObjects::RebinnedOutput;
using DataObjects::RebinnedOutput_sptr;
using Mantid::MantidVec;
using Mantid::MantidVecPtr;

//------------------------------------------------------------------------------
// Public member functions
//------------------------------------------------------------------------------
/**
 * Default constructor
 */
XDataConverter::XDataConverter() : m_sharedX(false) {}

//------------------------------------------------------------------------------
// Private member functions
//------------------------------------------------------------------------------

/// Initialize the properties on the algorithm
void XDataConverter::init() {
  using Kernel::Direction;
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "Name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace, can be the same as the input.");
}

/// Execute the algorithm
void XDataConverter::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if (!isProcessingRequired(inputWS)) {
    setProperty("OutputWorkspace", inputWS);
    return;
  }

  const auto numSpectra = static_cast<int>(inputWS->getNumberHistograms());
  const size_t numYValues = getNewYSize(inputWS);
  const size_t numXValues = getNewXSize(numYValues);
  m_sharedX = API::WorkspaceHelpers::sharedXData(inputWS);
  // Create the new workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, numSpectra, numXValues, numYValues);

  // Copy over the 'vertical' axis
  if (inputWS->axes() > 1)
    outputWS->replaceAxis(1, std::unique_ptr<API::Axis>(inputWS->getAxis(1)->clone(outputWS.get())));

  bool isRebinnedWorkspace = inputWS->id() == "RebinnedOutput";
  RebinnedOutput_sptr outRB = std::dynamic_pointer_cast<RebinnedOutput>(outputWS);
  // Force fractions to unity (converting from histo to point discards bin info).

  Progress prog(this, 0.0, 1.0, numSpectra);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int i = 0; i < int(numSpectra); ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // Copy over the Y and E data
    outputWS->setSharedY(i, inputWS->sharedY(i));
    outputWS->setSharedE(i, inputWS->sharedE(i));
    if (isRebinnedWorkspace && outRB) {
      MantidVecPtr outF;
      outF.access().resize(inputWS->getNumberBins(i), 1.0);
      outRB->setF(i, outF);
    }
    setXData(outputWS, inputWS, i);
    if (inputWS->hasDx(i)) {
      outputWS->setSharedDx(i, inputWS->sharedDx(i));
    }
    prog.report();

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Store the output
  setProperty("OutputWorkspace", outputWS);
}

std::size_t XDataConverter::getNewYSize(const API::MatrixWorkspace_sptr &inputWS) {
  // this is the old behavior of MatrixWorkspace::blocksize()
  return inputWS->y(0).size();
}

/**
 * Set the X data on given spectra
 * @param outputWS :: The destination workspace
 * @param inputWS :: The input workspace
 * @param index :: The index
 */
void XDataConverter::setXData(const API::MatrixWorkspace_sptr &outputWS, const API::MatrixWorkspace_sptr &inputWS,
                              const int index) {
  if (m_sharedX) {
    PARALLEL_CRITICAL(XDataConverter_para) {
      if (!m_cachedX) {
        PARALLEL_CRITICAL(XDataConverter_parb) { m_cachedX = calculateXPoints(inputWS->sharedX(index)); }
      }
    }
    outputWS->setSharedX(index, m_cachedX);
  } else {
    outputWS->setSharedX(index, calculateXPoints(inputWS->sharedX(index)));
  }
}
} // namespace Mantid::Algorithms
