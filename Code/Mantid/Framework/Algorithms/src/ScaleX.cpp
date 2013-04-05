/*WIKI* 

Scales the X axis of the input workspace by the amount requested. 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ScaleX.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
 namespace Algorithms
 {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ScaleX)

/// Sets documentation strings for this algorithm
void ScaleX::initDocs()
{
  this->setWikiSummary("Scales an input workspace by the given factor, which can be either multiplicative or additive.");
  this->setOptionalMessage("Scales an input workspace by the given factor, which can be either multiplicative or additive.");
}

/**
 * Default constructor
 */
ScaleX::ScaleX() : API::Algorithm(), m_progress(NULL) {}

/**
 * Destructor
 */
ScaleX::~ScaleX()
{
  delete m_progress;
}

/**
 * Initialisation method. Declares properties to be used in algorithm.
 */
void ScaleX::init()
{
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "Name of the input workspace");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace");
  auto isDouble = boost::make_shared<BoundedValidator<double> >();
  declareProperty("Factor", 1.0, isDouble, "The value by which to scale the input workspace. Default is 1.0");
  std::vector<std::string> op(2);
  op[0] = "Multiply";
  op[1] = "Add";
  declareProperty("Operation","Multiply",boost::make_shared<StringListValidator>(op),"Whether to multiply by, or add factor");
  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);
  declareProperty("IndexMin", 0, mustBePositive, "The workspace index of the first spectrum to scale. Only used if IndexMax is set.");
  declareProperty("IndexMax", Mantid::EMPTY_INT(), mustBePositive, "The workspace index of the last spectrum to scale. Only used if explicitly set.");
}

/**
 * Executes the algorithm
 */
void ScaleX::exec()
{
  //Get input workspace and offset
  const MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
  factor = getProperty("Factor");
  const std::string op = getPropertyValue("Operation");
  API::MatrixWorkspace_sptr outputW = createOutputWS(inputW);
  //Get number of histograms
  int histnumber = static_cast<int>(inputW->getNumberHistograms());
  m_progress = new API::Progress(this, 0.0, 1.0, histnumber+1);
  m_progress->report("Scaling X");
  wi_min = 0;
  wi_max = histnumber-1;
  //check if workspace indexes have been set
  int tempwi_min = getProperty("IndexMin");
  int tempwi_max = getProperty("IndexMax");
  if ( tempwi_max != Mantid::EMPTY_INT() )
  {
    if ((wi_min <= tempwi_min) && (tempwi_min <= tempwi_max) && (tempwi_max <= wi_max))
    {
      wi_min = tempwi_min;
      wi_max = tempwi_max;
    }
    else
    {
      g_log.error("Invalid Workspace Index min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }
  //Check if its an event workspace
  EventWorkspace_const_sptr eventWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputW);
  if (eventWS != NULL)
  {
    this->execEvent();
    return;
  }
  // do the shift in X
  PARALLEL_FOR2(inputW, outputW)
  for (int i=0; i < histnumber; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    //Do the offsetting
    for (int j=0; j <  static_cast<int>(inputW->readX(i).size()); ++j)
    {
      //Change bin value by offset
      if ((i >= wi_min) && (i <= wi_max))
      {
        if(op=="Multiply")
        {
          outputW->dataX(i)[j] = inputW->readX(i)[j] * factor;
        }
        else if(op=="Add")
        {
          outputW->dataX(i)[j] = inputW->readX(i)[j] + factor;
        }
      }
      else outputW->dataX(i)[j] = inputW->readX(i)[j];
    }
    //Copy y and e data
    outputW->dataY(i) = inputW->dataY(i);
    outputW->dataE(i) = inputW->dataE(i);
    // reverse the vector if multiplicative factor was negative
    if( (i >= wi_min) && (i <= wi_max) && op=="Multiply" && factor<0 )
    {
      std::reverse( outputW->dataX(i).begin(), outputW->dataX(i).end() );
      std::reverse( outputW->dataY(i).begin(), outputW->dataY(i).end() );
      std::reverse( outputW->dataE(i).begin(), outputW->dataE(i).end() );
    }
    m_progress->report("Scaling X");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  // Copy units
  if (outputW->getAxis(0)->unit().get())
    outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
  try
  {
    if (inputW->getAxis(1)->unit().get())
      outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
  }
  catch(Exception::IndexError &)
  {
    // OK, so this isn't a Workspace2D
  }
  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputW);
}

API::MatrixWorkspace_sptr ScaleX::createOutputWS(API::MatrixWorkspace_sptr input)
{
  //Check whether input = output to see whether a new workspace is required.
  MatrixWorkspace_sptr output = getProperty("OutputWorkspace");
  if ( input != output )
  {
    //Create new workspace for output from old
    output = API::WorkspaceFactory::Instance().create(input);
  }
  return output;
 }

void ScaleX::execEvent()
{
  g_log.information("Processing event workspace");
  const MatrixWorkspace_const_sptr matrixInputWS = this->getProperty("InputWorkspace");
  const std::string op = getPropertyValue("Operation");
  EventWorkspace_const_sptr inputWS=boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);
  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS)
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  else
  {
    //Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    //You need to copy over the data as well.
    outputWS->copyDataFrom( (*inputWS) );
    //Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }
  int numHistograms = static_cast<int>(inputWS->getNumberHistograms());
  PARALLEL_FOR1(outputWS)
  for (int i=0; i < numHistograms; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    //Do the offsetting
    if ((i >= wi_min) && (i <= wi_max))
    {
      if(op=="Multiply")
      {
        outputWS->getEventList(i).scaleTof(factor);
        if( factor < 0 )
        {
          outputWS->getEventList(i).reverse();
        }
      }
      else if(op=="Add")
      {
        outputWS->getEventList(i).addTof(factor);
      }
    }
    m_progress->report("Scaling X");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  outputWS->clearMRU();
}

} // namespace Algorithm
} // namespace Mantid




