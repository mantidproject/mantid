//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/XDataConverter.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace Algorithms {

using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace_const_sptr;
using API::WorkspaceFactory;
using API::Progress;
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
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                           Direction::Input),
                  "Name of the input workspace.");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
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
  const size_t numYValues = inputWS->blocksize();
  const size_t numXValues = getNewXSize(inputWS);
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
    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Store the output
  setProperty("OutputWorkspace", outputWS);
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
}
}
