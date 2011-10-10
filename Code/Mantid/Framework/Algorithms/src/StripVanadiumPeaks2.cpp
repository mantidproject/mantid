#include "MantidAlgorithms/StripVanadiumPeaks2.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(StripVanadiumPeaks2)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  StripVanadiumPeaks2::StripVanadiumPeaks2()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  StripVanadiumPeaks2::~StripVanadiumPeaks2()
  {
    // TODO Auto-generated destructor stub
  }
  
  void StripVanadiumPeaks2::init(){
    // Declare inputs and output.  Copied from StripPeaks

    declareProperty(
      new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
      "Name of the input workspace. If you use the default vanadium peak positions are used, the workspace must be in units of d-spacing." );

    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
      "The name of the workspace to be created as the output of the algorithm.\n"
      "If the input workspace is an EventWorkspace, then the output must be different (and will be made into a Workspace2D)." );

    BoundedValidator<int> *min = new BoundedValidator<int>();
    min->setLower(1.0);
    // The estimated width of a peak in terms of number of channels
    declareProperty("FWHM", 7, min,
      "Estimated number of points covered by the fwhm of a peak (default 7)" );

    // The tolerance allowed in meeting the conditions
    declareProperty("Tolerance",4, min->clone(),
      "A measure of the strictness desired in meeting the condition on peak candidates,\n"
      "Mariscotti recommends 2 (default 4)");

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("WorkspaceIndex",EMPTY_INT(),mustBePositive,
      "If set, peaks will only be removed from this spectrum (otherwise from all)");

    return;

  }

  void StripVanadiumPeaks2::exec(){

    // 1. Process input/output
    API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    std::string outputWSName = getPropertyValue("OutputWorkspace");
    int singleIndex = getProperty("WorkspaceIndex");
    int param_fwhm = getProperty("FWHM");
    int param_tolerance = getProperty("Tolerance");

    bool singleSpectrum = !isEmpty(singleIndex);

    // 2. Call StripPeaks
    std::string peakpositions = "0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401";

    IAlgorithm_sptr alg1 = createSubAlgorithm("StripPeaks");
    alg1->setProperty("InputWorkspace", inputWS);
    alg1->setPropertyValue("OutputWorkspace", outputWSName);
    alg1->setProperty("FWHM", param_fwhm);
    alg1->setProperty("Tolerance", param_tolerance);
    alg1->setProperty("PeakPositions", peakpositions);
    if (singleSpectrum){
      alg1->setProperty("WorkspaceIndex", singleIndex);
    }

    alg1->executeAsSubAlg();

    // 3. Get and set output workspace
    // API::MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<API::MatrixWorkspace_sptr>(AnalysisDataService::Instance().retrieve(outputWSName));
    // boost::shared_ptr<API::Workspace> outputWS = AnalysisDataService::Instance().retrieve(outputWSName);
    API::MatrixWorkspace_sptr outputWS = alg1->getProperty("OutputWorkspace");

    this->setProperty("OutputWorkspace", outputWS);

    return;
  }



} // namespace Mantid
} // namespace Algorithms

