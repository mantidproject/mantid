//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumRowColumn.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SumRowColumn)

/// Sets documentation strings for this algorithm
void SumRowColumn::initDocs()
{
  this->setWikiSummary("SANS-specific algorithm which gives a single spectrum containing the total counts in either each row or each column of pixels in a square LOQ or SANS2D detector bank. ");
  this->setOptionalMessage("SANS-specific algorithm which gives a single spectrum containing the total counts in either each row or each column of pixels in a square LOQ or SANS2D detector bank.");
}


using namespace Mantid::Kernel;
using namespace Mantid::API;

void SumRowColumn::init()
{
  // Assume input workspace has correct spectra in it - no more and no less
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  // Need to select whether to sum rows or columns
  std::vector<std::string> orientation;
  orientation.push_back("D_H");
  orientation.push_back("D_V");
  declareProperty("Orientation","",new ListValidator(orientation));

  // This is the range to select - the whole lot by default
  declareProperty("XMin",EMPTY_DBL());
  declareProperty("XMax",EMPTY_DBL());

  // For selecting a column range - the whole lot by default
  BoundedValidator<int>* mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("HoverV_Min",EMPTY_INT(),mustBePositive);
  declareProperty("HoverV_Max",EMPTY_INT(),mustBePositive->clone());
}

void SumRowColumn::exec()
{
  // First task is to integrate the input workspace
  MatrixWorkspace_const_sptr integratedWS = integrateWorkspace();

  const int numSpec = integratedWS->getNumberHistograms();
  // Check number of spectra is 128*128 or 192*192. Print warning if not.
  if (numSpec != 16384 && numSpec != 36864)
  {
    g_log.warning() << "The input workspace has " << numSpec << " spectra."
      << "This is not 128*128 or 192*192 - did you make a mistake?\n";
  }

  // This is the dimension if all rows/columns are included
  const int dim = static_cast<int>( std::sqrt(static_cast<double>(numSpec)) );

  // Check the column range properties
  int start = getProperty("HoverV_Min");
  int end = getProperty("HoverV_Max");
  if ( isEmpty(start) ) start = 0;
  if ( isEmpty(end) || end > dim-1 ) end = dim-1;
  if ( start > end )
  {
    g_log.error("H/V_Min must be less than H/V_Max");
    throw std::out_of_range("H/V_Min must be less than H/V_Max");
  }

  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(integratedWS,1,dim,dim);
  // Remove the unit
  outputWS->getAxis(0)->unit().reset();

  // Get references to the vectors for the results
  MantidVec& X = outputWS->dataX(0);
  MantidVec& Y = outputWS->dataY(0);

  // Get the orientation
  const std::string orientation = getProperty("Orientation");
  const bool horizontal = ( orientation=="D_H" ? 1 : 0 );

  Progress progress(this,0,1,dim);
  for (int i = 0; i < dim; ++i)
  {
    // Copy X values
    X[i] = i;

    // Now loop over calculating Y's
    for (int j = start; j <= end; ++j)
    {
      const int index = ( horizontal ? ( i + j*dim) : ( i*dim + j) );
      Y[i] += integratedWS->readY(index)[0];
    }
  }

  setProperty("OutputWorkspace",outputWS);
}

/** Call Integration as a sub-algorithm
 *  @return The integrated workspace
 */
MatrixWorkspace_sptr SumRowColumn::integrateWorkspace()
{
  g_log.debug() << "Integrating input workspace\n";

  IAlgorithm_sptr childAlg = createSubAlgorithm("Integration");
  //pass inputed values straight to this sub-algorithm, checking must be done there
  childAlg->setPropertyValue( "InputWorkspace", getPropertyValue("InputWorkspace") );
  childAlg->setPropertyValue( "RangeLower",  getPropertyValue("XMin") );
  childAlg->setPropertyValue( "RangeUpper", getPropertyValue("XMax") );

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run Integration sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    const std::string mess("Unable to successfully run Integration sub-algorithm");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }

  return childAlg->getProperty("OutputWorkspace");
}

} // namespace Algorithms
} // namespace Mantid
