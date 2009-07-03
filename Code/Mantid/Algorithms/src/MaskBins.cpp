//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MaskBins.h"
#include <limits>
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskBins)

using namespace Kernel;
using namespace API;

MaskBins::MaskBins() : API::Algorithm(), m_startX(0.0), m_endX(0.0) {}

void MaskBins::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  
  // This validator effectively makes these properties mandatory
  // Would be nice to have an explicit validator for this, but MandatoryValidator is already taken!
  BoundedValidator<double> *required = new BoundedValidator<double>();
  required->setUpper(std::numeric_limits<double>::max()*0.99);
  declareProperty("XMin",std::numeric_limits<double>::max(),required);
  declareProperty("XMax",std::numeric_limits<double>::max(),required->clone());
}

/** Execution code.
 *  @throw std::invalid_argument If XMax is less than XMin
 */
void MaskBins::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  
  m_startX = getProperty("XMin");
  m_endX = getProperty("XMax");
  MantidVec::difference_type startBin(0),endBin(0);

  if (m_startX > m_endX)
  {
    const std::string failure("XMax must be greater than XMin.");
    g_log.error(failure);
    throw std::invalid_argument(failure);
  }
  
  // If the binning is the same throughout, we only need to find the index limits once
  const bool commonBins = WorkspaceHelpers::commonBoundaries(inputWS);
  if (commonBins)
  {
    const MantidVec& X = inputWS->readX(0);
    this->findIndices(X,startBin,endBin);
  }
  
  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }
  
  const int numHists = inputWS->getNumberHistograms();
  for (int i = 0; i < numHists; ++i)
  {
    // Copy over the data
    const MantidVec& X = outputWS->dataX(i) = inputWS->readX(i);
    outputWS->dataY(i) = inputWS->readY(i);
    outputWS->dataE(i) = inputWS->readE(i);
    
    if (!commonBins) this->findIndices(X,startBin,endBin);
    
    // Loop over masking each bin in the range
    for (int j = startBin; j < endBin; ++j)
    {
      outputWS->maskBin(i,j);
    }
  }
 
}

/** Finds the indices of the bins at the limits of the range given. 
 *  @param X        The X vector to search
 *  @param startBin Returns the bin index including the starting value
 *  @param endBin   Returns the bin index after the end value
 */
void MaskBins::findIndices(const MantidVec& X, MantidVec::difference_type& startBin, MantidVec::difference_type& endBin)
{
  startBin = std::upper_bound(X.begin(),X.end(),m_startX) - X.begin();
  if (startBin!=0) --startBin;
  MantidVec::const_iterator last = std::lower_bound(X.begin(),X.end(),m_endX);
  if (last==X.end()) --last;
  endBin = last - X.begin();
}

} // namespace Algorithms
} // namespace Mantid

