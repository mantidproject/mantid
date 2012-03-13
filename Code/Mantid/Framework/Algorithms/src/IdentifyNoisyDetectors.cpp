/*WIKI* 

The process for this algorithm is:
* The standard deviation for each pixel within the pre-set range is calculated.
* The mean value and standard deviation of these standard deviation values is calculated.
* Any detector/pixel for which it's standard deviation value satisfied the following conditions:
** sdev(pixel) < mean(sdevs) - 3 * sdev(sdevs)
** sdev(pixel) > mean(sdevs) + 3 * sdev(sdevs)
** sdev(pixel) < mean(sdevs) * 0.0001
* (cont) is considered to be "noisy".

This is repeated three times from the second step.

This uses the [[Integration]], [[Power]] and [[Divide]] algorithms for the first step.

The lower bound for the integration is currently fixed to 2000.

The upper bound for the integration is currently fixed to 19000.


*WIKI*/
#include "MantidAlgorithms/IdentifyNoisyDetectors.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{
using namespace Kernel;
using namespace API;

DECLARE_ALGORITHM(IdentifyNoisyDetectors)

/// Sets documentation strings for this algorithm
void IdentifyNoisyDetectors::initDocs()
{
  this->setWikiSummary("This algorithm creates a single-column workspace where the Y values are populated withs 1s and 0s, 0 signifying that the detector is to be considered \"bad\" based on the method described below. ");
  this->setOptionalMessage("This algorithm creates a single-column workspace where the Y values are populated withs 1s and 0s, 0 signifying that the detector is to be considered 'bad' based on the method described below.");
}


void IdentifyNoisyDetectors::init()
{
  auto wsVal = boost::make_shared<CompositeValidator>();
  wsVal->add<WorkspaceUnitValidator>("TOF");
  wsVal->add<HistogramValidator>();
  wsVal->add<SpectraAxisValidator>();
  wsVal->add<InstrumentValidator>();
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,wsVal));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));
}

void IdentifyNoisyDetectors::exec()
{
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  const int nHist = static_cast<int>(inputWS->getNumberHistograms());

  const double rangeLower = 2000;
  const double rangeUpper = 19000;
  const double steps = rangeUpper - rangeLower;
  
  // Create the output workspace a single value for each spectra.
  MatrixWorkspace_sptr outputWs;
  outputWs = WorkspaceFactory::Instance().create(inputWS, nHist, 1, 1);

  MatrixWorkspace_sptr stdDevWs;
  stdDevWs = WorkspaceFactory::Instance().create(outputWs);

  IAlgorithm_sptr integ = createSubAlgorithm("Integration");  
  integ->initialize();
  integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWs);
  integ->setProperty<double>("RangeLower", rangeLower);
  integ->setProperty<double>("RangeUpper", rangeUpper);
  integ->execute();

  MatrixWorkspace_sptr int1 = integ->getProperty("OutputWorkspace");
  
  IAlgorithm_sptr power = createSubAlgorithm("Power");
  power->initialize();
  power->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWs);
  power->setProperty<double>("Exponent", 2.0);
  power->execute();

  MatrixWorkspace_sptr power_tmp = power->getProperty("OutputWorkspace");
  
  // integrate again
  integ = createSubAlgorithm("Integration");
  integ->initialize();
  integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", power_tmp);
  integ->setProperty<double>("RangeLower", rangeLower);
  integ->setProperty<double>("RangeUpper", rangeUpper);
  integ->execute();

  MatrixWorkspace_sptr int2 = integ->getProperty("OutputWorkspace");
  
  IAlgorithm_sptr csvw = createSubAlgorithm("CreateSingleValuedWorkspace");
  csvw->initialize();
  csvw->setProperty<double>("DataValue", steps);
  csvw->execute();

  MatrixWorkspace_sptr stepsWs = csvw->getProperty("OutputWorkspace");

  IAlgorithm_sptr divide = createSubAlgorithm("Divide");
  divide->initialize();
  divide->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", int1);
  divide->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", stepsWs);
  divide->execute();
  
  int1 = divide->getProperty("OutputWorkspace");

  divide = createSubAlgorithm("Divide");
  divide->initialize();
  divide->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", int2);
  divide->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", stepsWs);
  divide->execute();

  int2 = divide->getProperty("OutputWorkspace");

  for ( int i = 0; i < nHist; i++ )
  {
    stdDevWs->dataX(i)[0] = 0.0;
    stdDevWs->dataY(i)[0] = sqrt ( int2->readY(i)[0] - std::pow(int1->readY(i)[0], 2) );
    outputWs->dataX(i)[0] = 0.0;
    outputWs->dataY(i)[0] = 1.0;
  }

  getStdDev(outputWs, stdDevWs);
  getStdDev(outputWs, stdDevWs);
  getStdDev(outputWs, stdDevWs);

  setProperty("OutputWorkspace", outputWs);
}

/**
* Main work portion of algorithm. Calculates mean of standard deviation, ignoring
* the detectors marked as "bad", then determines if any of the detectors are "bad".
* @param valid :: eventual output workspace, holding 0 for bad and 1 for good
* @param values :: stddeviations of each spectra (I think)
*/
void IdentifyNoisyDetectors::getStdDev(MatrixWorkspace_sptr valid, MatrixWorkspace_sptr values)
{
  const int nhist = static_cast<int>(valid->getNumberHistograms());
  int count = 0;
  double mean = 0.0;
  double mean2 = 0.0;

  for ( int i = 0; i < nhist; i++ )
  {
    if ( valid->readY(i)[0] > 0 )
    {
      mean += values->readY(i)[0];
      mean2 += std::pow(values->readY(i)[0], 2);
      count++;
    }
  }

  mean = mean / count;
  double stddev = sqrt( (mean2/count) - std::pow(mean, 2) );

  double upper = mean + 3 * stddev;
  double lower = mean - 3 * stddev;
  double min = mean * 0.0001;

  for ( int i = 0; i < nhist; i++ )
  {
    double value = values->readY(i)[0];
    if ( value > upper ) { valid->dataY(i)[0] = 0.0; }
    else if ( value < lower ) { valid->dataY(i)[0] = 0.0; }
    else if ( value < min ) { valid->dataY(i)[0] = 0.0; }
  }
}

} // namespace Algorithms
} // namespace Mantid
