// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/XDataConverter.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace Algorithms {

using API::MatrixWorkspace_const_sptr;
using API::MatrixWorkspace_sptr;
using API::Progress;
using API::WorkspaceFactory;
using API::WorkspaceProperty;
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
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "Name of the input workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
      "Name of the output workspace, can be the same as the input.");
}

/// Execute the algorithm
void XDataConverter::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if (!isProcessingRequired(inputWS)) {
    setProperty("OutputWorkspace", inputWS);
    return;
  }

  const int numSpectra = static_cast<int>(inputWS->getNumberHistograms());
  const size_t numYValues = getNewYSize(inputWS);
  const size_t numXValues = getNewXSize(numYValues);
  m_sharedX = API::WorkspaceHelpers::sharedXData(*inputWS);
  // Create the new workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(
      inputWS, numSpectra, numXValues, numYValues);

  // Copy over the 'vertical' axis
  if (inputWS->axes() > 1)
    outputWS->replaceAxis(1, inputWS->getAxis(1)->clone(outputWS.get()));

  Progress prog(this, 0.0, 1.0, numSpectra);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int i = 0; i < int(numSpectra); ++i) {
    PARALLEL_START_INTERUPT_REGION

    // Copy over the Y and E data
    outputWS->setSharedY(i, inputWS->sharedY(i));
    outputWS->setSharedE(i, inputWS->sharedE(i));
    setXData(outputWS, inputWS, i);
    if (inputWS->hasDx(i)) {
      outputWS->setSharedDx(i, inputWS->sharedDx(i));
    }
    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Store the output
  setProperty("OutputWorkspace", outputWS);
}

std::size_t
XDataConverter::getNewYSize(const API::MatrixWorkspace_sptr inputWS) {
  // this is the old behavior of MatrixWorkspace::blocksize()
  return inputWS->y(0).size();
}

/**
 * Set the X data on given spectra
 * @param outputWS :: The destination workspace
 * @param inputWS :: The input workspace
 * @param index :: The index
 */
void XDataConverter::setXData(API::MatrixWorkspace_sptr outputWS,
                              const API::MatrixWorkspace_sptr inputWS,
                              const int index) {
  if (m_sharedX) {
    PARALLEL_CRITICAL(XDataConverter_para) {
      if (!m_cachedX) {
        PARALLEL_CRITICAL(XDataConverter_parb) {
          m_cachedX = calculateXPoints(inputWS->sharedX(index));
        }
      }
    }
    outputWS->setSharedX(index, m_cachedX);
  } else {
    outputWS->setSharedX(index, calculateXPoints(inputWS->sharedX(index)));
  }
}
} // namespace Algorithms
} // namespace Mantid
