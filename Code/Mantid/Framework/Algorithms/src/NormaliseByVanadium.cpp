#include "MantidAlgorithms/NormaliseByVanadium.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(NormaliseByVanadium)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  NormaliseByVanadium::NormaliseByVanadium() : Mantid::API::Algorithm()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  NormaliseByVanadium::~NormaliseByVanadium()
  {
  }
  

  /// Initalise the documentation
  void NormaliseByVanadium::initDocs()
  {
    this->setWikiSummary("Normalise a sample workspace by a vanadium workspace. Use nearest neighbours for averaging vanadium pixels.");
    this->setOptionalMessage("Normalise a sample workspace by a vanadium workspace. Use nearest neighbours for averaging vanadium pixels.");
  }

  /// Declare the properties
  void NormaliseByVanadium::init()
  {
    declareProperty(new WorkspaceProperty<>("SampleInputWorkspace","",Direction::Input,new HistogramValidator<>),
      "The name of the sample Workspace2D to take as input");
    declareProperty(new WorkspaceProperty<>("VanadiumInputWorkspace","",Direction::Input,new HistogramValidator<>),
      "The name of the vanadium Workspace2D to take as input");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
      "The name of the workspace in which to store the result");
  }

  /// Run the algorithm
  void NormaliseByVanadium::exec()
  {
    MatrixWorkspace_sptr sampleWS = getProperty("SampleInputWorkspace");
    MatrixWorkspace_sptr vanadiumWS = getProperty("VanadiumInputWorkspace");
    
    // Integrate across all bins
    IAlgorithm_sptr integrateAlg = this->createSubAlgorithm("Integration", 0, 1, true, 1);
    integrateAlg->setProperty("InputWorkspace", vanadiumWS);
    integrateAlg->setPropertyValue("OutputWorkspace", "VanSumTOF");
    integrateAlg->setProperty("IncludePartialBins", true);
    integrateAlg->executeAsSubAlg();
    MatrixWorkspace_sptr vanSumTOF_WS = integrateAlg->getProperty("OutputWorkspace");

    // Calculate the average TOF accross all spectra
    size_t nSpectra = vanSumTOF_WS->getNumberHistograms();
    Progress progress(this,0.0,1.0,nSpectra);
    double spectraSum = 0;
    //OPENMP
    for(size_t i = 0; i < nSpectra; i++)
    {
      spectraSum += vanSumTOF_WS->readY(i)[0];
      progress.report();
    }
    double yavg =  spectraSum/nSpectra;

    // Normalise the integrated TOFs
    IAlgorithm_sptr createDenominatorAlg = this->createSubAlgorithm("CreateSingleValuedWorkspace", 0, 1, true, 1);
    createDenominatorAlg->setProperty("DataValue", yavg);
    createDenominatorAlg->setPropertyValue("OutputWorkspace", "denominator");
    createDenominatorAlg->executeAsSubAlg();
    MatrixWorkspace_sptr denominatorWS = createDenominatorAlg->getProperty("OutputWorkspace");
    vanSumTOF_WS = vanSumTOF_WS/denominatorWS;

    //Mask detectors outside of limits
    IAlgorithm_sptr constrainAlg = this->createSubAlgorithm("FindDetectorsOutsideLimits", 0, 1, true, 1);
    constrainAlg->setProperty("InputWorkspace", vanSumTOF_WS);
    constrainAlg->setPropertyValue("OutputWorkspace", "ConstrainedWS");
    constrainAlg->setProperty("HighThreshold", pow((double)10, (double)300));
    constrainAlg->setProperty("LowThreshold", pow((double)10, -(double)300));
    constrainAlg->executeAsSubAlg();
    MatrixWorkspace_sptr constrainedWS = constrainAlg->getProperty("OutputWorkspace");

    //Mask detectors outside of limits
    IAlgorithm_sptr maskdetectorsAlg = this->createSubAlgorithm("MaskDetectors", 0, 1, true, 1);
    maskdetectorsAlg->setProperty("Workspace", vanSumTOF_WS);
    maskdetectorsAlg->setProperty("MaskedWorkspace", constrainedWS);
    maskdetectorsAlg->executeAsSubAlg();

    sampleWS = sampleWS / vanSumTOF_WS;

    setProperty("OutputWorkspace", sampleWS);
  }


} // namespace Mantid
} // namespace Algorithms

