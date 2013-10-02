/*WIKI* 

Scales the X axis of the input workspace by the amount requested. The amount can be specified either as:
* an absolute numerical value via the "Factor" argument or
* an detector parameter name whose value is retrieved from the instrument.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ScaleX.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include <boost/function.hpp>

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
  //Add InstrumentParameter property here so as not to mess with the parameter order for current scripts
  declareProperty("InstrumentParameter", "", "The name of an instrument parameter whose value is used to scale as the input factor");
}

/**
 * Executes the algorithm
 */
void ScaleX::exec()
{
  //Get input workspace and offset
  const MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
  m_factor = getProperty("Factor");
  m_parname = getPropertyValue("InstrumentParameter");

  const std::string op = getPropertyValue("Operation");
  API::MatrixWorkspace_sptr outputW = createOutputWS(inputW);
  //Get number of histograms
  int histnumber = static_cast<int>(inputW->getNumberHistograms());
  m_progress = new API::Progress(this, 0.0, 1.0, histnumber+1);
  m_progress->report("Scaling X");
  m_wi_min = 0;
  m_wi_max = histnumber-1;
  //check if workspace indexes have been set
  int tempwi_min = getProperty("IndexMin");
  int tempwi_max = getProperty("IndexMax");
  if ( tempwi_max != Mantid::EMPTY_INT() )
  {
    if ((m_wi_min <= tempwi_min) && (tempwi_min <= tempwi_max) && (tempwi_max <= m_wi_max))
    {
      m_wi_min = tempwi_min;
      m_wi_max = tempwi_max;
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

  const bool multiply = (op=="Multiply");
  boost::function<double (double x,double factor)> scale;
  if(multiply) scale = std::multiplies<double>();
  else scale = std::plus<double>();

  // do the shift in X
  PARALLEL_FOR2(inputW, outputW)
  for (int i = 0; i < histnumber; ++i)
  {
    PARALLEL_START_INTERUPT_REGION

    //Copy y and e data
    auto & outY = outputW->dataY(i);
    outY = inputW->dataY(i);
    auto & outE = outputW->dataE(i);
    outE = inputW->dataE(i);

    auto & outX = outputW->dataX(i);
    const auto & inX = inputW->readX(i);
    //Change bin value by offset
    if ((i >= m_wi_min) && (i <= m_wi_max))
    {
      double factor = getScaleFactor(inputW, i);
      // Do the offsetting
      std::transform(inX.begin(), inX.end(), outX.begin(), std::bind2nd(scale, factor));
      // reverse the vector if multiplicative factor was negative
      if(multiply && m_factor < 0.0)
      {
        std::reverse( outX.begin(), outX.end() );
        std::reverse( outY.begin(), outY.end() );
        std::reverse( outE.begin(), outE.end() );
      }
    }
    else
    {
      outX = inX; //copy
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
    if ((i >= m_wi_min) && (i <= m_wi_max))
    {
      if(op=="Multiply")
      {
        outputWS->getEventList(i).scaleTof(getScaleFactor(inputWS, i));
        if( m_factor < 0 )
        {
          outputWS->getEventList(i).reverse();
        }
      }
      else if(op=="Add")
      {
        outputWS->getEventList(i).addTof(getScaleFactor(inputWS, i));
      }
    }
    m_progress->report("Scaling X");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  outputWS->clearMRU();
}

API::MatrixWorkspace_sptr ScaleX::createOutputWS(const API::MatrixWorkspace_sptr & input)
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


/**
 * If the InstrumentParameter property is set then it attempts to retrieve the parameter
 * from the component, else it returns the value of the Factor property
 * @param inputWS A pointer to the input workspace
 * @param index The current index to inspect
 * @return Value for the scale factor
 */
double ScaleX::getScaleFactor(const API::MatrixWorkspace_const_sptr & inputWS, const size_t index)
{
  if(m_parname.empty()) return m_factor;

  // Try and get factor from component. If we see a DetectorGroup use this will use the first component
  Geometry::IDetector_const_sptr det;
  auto inst = inputWS->getInstrument();

  auto *spec = inputWS->getSpectrum(index);
  const auto & ids = spec->getDetectorIDs();
  const size_t ndets(ids.size());
  if(ndets > 0)
  {
    try
    {
      det = inst->getDetector(*ids.begin());
    }
    catch(Exception::NotFoundError&)
    {
      return 0.0;
    }
  }
  else return 0.0;

  const auto & pmap = inputWS->constInstrumentParameters();
  auto par = pmap.getRecursive(det->getComponentID(), m_parname);
  if(par) return par->value<double>();
  else
  {
    std::ostringstream os;
    os << "Spectrum at index '" << index << "' has no parameter named '" << m_parname << "'\n";
    throw std::runtime_error(os.str());
  }
}


} // namespace Algorithm
} // namespace Mantid




