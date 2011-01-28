//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MaskBins.h"
#include <limits>
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskBins)

using namespace Kernel;
using namespace API;
using namespace Mantid;
using Mantid::DataObjects::EventList;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::DataObjects::EventWorkspace_const_sptr;

MaskBins::MaskBins() : API::Algorithm(), m_startX(0.0), m_endX(0.0) {}

void MaskBins::init()
{
  this->setOptionalMessage(
      "Mask out bins between two X limits.\n"
      "For EventWorkspaces, this deletes the events within that range."
      );

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  
  // This validator effectively makes these properties mandatory
  // Would be nice to have an explicit validator for this, but MandatoryValidator is already taken!
  BoundedValidator<double> *required = new BoundedValidator<double>();
  required->setUpper(std::numeric_limits<double>::max()*0.99);
  declareProperty("XMin",std::numeric_limits<double>::max(),required);
  declareProperty("XMax",std::numeric_limits<double>::max(),required->clone());

  // which pixels to load
  this->declareProperty(new ArrayProperty<int>("SpectraList"),
                        "Optional: A list of individual which spectra to mask (specified using the workspace index). If not set, all spectra are masked.");

}

/** Execution code.
 *  @throw std::invalid_argument If XMax is less than XMin
 */
void MaskBins::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Check for valid X limits
  m_startX = getProperty("XMin");
  m_endX = getProperty("XMax");

  if (m_startX > m_endX)
  {
    const std::string failure("XMax must be greater than XMin.");
    g_log.error(failure);
    throw std::invalid_argument(failure);
  }
  
  //---------------------------------------------------------------------------------
  // what spectra (workspace indices) to load. Optional.
  this->spectra_list = this->getProperty("SpectraList");
  if (this->spectra_list.size() > 0)
  {
    int numHist = inputWS->getNumberHistograms();
    //--- Validate spectra list ---
    for (size_t i = 0; i < this->spectra_list.size(); ++i)
    {
      int wi = this->spectra_list[i];
      if ((wi < 0) || (wi >= numHist))
      {
        std::ostringstream oss;
        oss << "One of the workspace indices specified, " << wi << " is above the number of spectra in the workspace (" << numHist <<").";
        throw std::invalid_argument(oss.str());
      }
    }
  }

  //---------------------------------------------------------------------------------
  //Now, determine if the input workspace is actually an EventWorkspace
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);

  if (eventW != NULL)
  {
    //------- EventWorkspace ---------------------------
    this->execEvent();
  }
  else
  {
    //------- MatrixWorkspace of another kind -------------
    MantidVec::difference_type startBin(0),endBin(0);

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
    Progress progress(this,0.0,1.0,numHists);
    //Parallel running has problems with a race condition, leading to occaisional test failures and crashes

    bool useSpectraList = (this->spectra_list.size() > 0);

    //Alter the for loop ending based on what we are looping on
    int for_end = numHists;
    if (useSpectraList) for_end = this->spectra_list.size();

    for (int i = 0; i < for_end; ++i)
    {
      // Find the workspace index, either based on the spectra list or all spectra
      int wi;
      if (useSpectraList)
        wi = this->spectra_list[i];
      else
        wi = i;

      // Copy over the data
      outputWS->dataX(wi) = inputWS->readX(wi);
      const MantidVec& X = outputWS->dataX(wi);
      outputWS->dataY(wi) = inputWS->readY(wi);
      outputWS->dataE(wi) = inputWS->readE(wi);

      MantidVec::difference_type startBinLoop(startBin),endBinLoop(endBin);
      if (!commonBins) this->findIndices(X,startBinLoop,endBinLoop);

      // Loop over masking each bin in the range
      for (int j = startBinLoop; j < endBinLoop; ++j)
      {
        outputWS->maskBin(wi,j);
      }
      progress.report();
    }

  }
 
}

/** Execution code for EventWorkspaces
 */
void MaskBins::execEvent()
{
  MatrixWorkspace_const_sptr inputMatrixWS = getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS = boost::dynamic_pointer_cast<const EventWorkspace>( inputMatrixWS );
  EventWorkspace_sptr outputWS;

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputMatrixWS = getProperty("OutputWorkspace");
  if ( outputMatrixWS != inputWS )
  {
    //Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    //outputWS->mutableSpectraMap().clear();
    //You need to copy over the data as well.
    outputWS->copyDataFrom( (*inputWS) );

    //Cast to the matrixOutputWS and save it
    MatrixWorkspace_sptr matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }
  else
  {
    //Output is same as input
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(outputMatrixWS);
  }

  //Go through all histograms
  const int numHists = inputWS->getNumberHistograms();
  Progress progress(this,0.0,1.0,numHists);

  if (this->spectra_list.size() > 0)
  {
    //Specific spectra were specified
    PARALLEL_FOR1(outputWS)
    for (int i = 0; i < static_cast<int>(this->spectra_list.size()); ++i)
    {
      PARALLEL_START_INTERUPT_REGION
      outputWS->getEventList( this->spectra_list[i] ).maskTof(m_startX, m_endX);
      progress.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }
  else
  {
    //Do all spectra!
    PARALLEL_FOR1(outputWS)
    for (int i = 0; i < numHists; ++i)
    {
      PARALLEL_START_INTERUPT_REGION
      outputWS->getEventList(i).maskTof(m_startX, m_endX);
      progress.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }


  //Clear the MRU
  outputWS->clearMRU();


}


/** Finds the indices of the bins at the limits of the range given. 
 *  @param X ::        The X vector to search
 *  @param startBin :: Returns the bin index including the starting value
 *  @param endBin ::   Returns the bin index after the end value
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

