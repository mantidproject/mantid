#include "MantidAlgorithms/NormaliseByVanadium.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Exception.h"

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
    if(sampleWS->getXDimension()->getNBins() != vanadiumWS->getXDimension()->getNBins())
    {
      throw std::runtime_error("Sample and Vanadium workspaces do not have the same number of bins.");
    }
    if(sampleWS->getNumberHistograms() != vanadiumWS->getNumberHistograms())
    {
      throw std::runtime_error("Sample and Vanadium workspaces do not have the same number of historams");
    }

    spec2index_map* specToWSIndexMap = vanadiumWS->getSpectrumToWorkspaceIndexMap();
    
    // Integrate across all bins
    IAlgorithm_sptr integrateAlg = this->createSubAlgorithm("Integration", 0, 0.01, true, 1);
    integrateAlg->setProperty("InputWorkspace", vanadiumWS);
    integrateAlg->setPropertyValue("OutputWorkspace", "VanSumTOF");
    integrateAlg->setProperty("IncludePartialBins", true);
    integrateAlg->executeAsSubAlg();
    MatrixWorkspace_sptr vanSumTOF_WS = integrateAlg->getProperty("OutputWorkspace");

    // Calculate the average TOF accross all spectra
    size_t nHistograms = vanSumTOF_WS->getNumberHistograms();
    Progress progress(this,0.01,0.99,nHistograms/10000);

    /// Create an empty workspace with the same dimensions as the integrated vanadium.
    MatrixWorkspace_sptr yAvgWS = Mantid::API::WorkspaceFactory::Instance().create(vanSumTOF_WS);

    Mantid::Geometry::IDetector_const_sptr idet;
    std::map<specid_t, double> specIdMap;
    std::map<specid_t, double>::iterator it;
    double spectraSum = 0;

    /*
    Find the nearest neighbours for the spectrum and use those to calculate an average (9 points)
    */
    //PARALLEL_FOR2(yAvgWS, vanSumTOF_WS)
    for(int i = 0; i < int(nHistograms); i++)
    {
      //PARALLEL_START_INTERUPT_REGION
      try
      {
        idet = vanadiumWS->getDetector(i);
        specIdMap = vanadiumWS->getNeighbours(idet.get());
        it = specIdMap.begin();
        while(it != specIdMap.end())
        {
          spectraSum += vanSumTOF_WS->readY((*specToWSIndexMap)[it->first])[0];
          it++;
        }
        spectraSum += vanSumTOF_WS->readY(i)[0];
        yAvgWS->dataY(i)[0] = spectraSum/(double(specIdMap.size() + 1));
      }
      catch(Kernel::Exception::NotFoundError&)
      {
      }
      progress.report();

      //PARALLEL_END_INTERUPT_REGION
    }
    //PARALLEL_CHECK_INTERUPT_REGION

    // Normalise the integrated TOFs
    vanSumTOF_WS = vanSumTOF_WS/yAvgWS;

    //Mask detectors outside of limits
    IAlgorithm_sptr constrainAlg = this->createSubAlgorithm("FindDetectorsOutsideLimits", 0.99, 0.991, true, 1);
    constrainAlg->setProperty("InputWorkspace", vanSumTOF_WS);
    constrainAlg->setPropertyValue("OutputWorkspace", "ConstrainedWS");
    constrainAlg->setProperty("HighThreshold", pow((double)10, (double)300));
    constrainAlg->setProperty("LowThreshold", pow((double)10, -(double)300));
    constrainAlg->executeAsSubAlg();
    MatrixWorkspace_sptr constrainedWS = constrainAlg->getProperty("OutputWorkspace");

    //Mask detectors outside of limits
    IAlgorithm_sptr maskdetectorsAlg = this->createSubAlgorithm("MaskDetectors", 0.991, 1, true, 1);
    maskdetectorsAlg->setProperty("Workspace", vanSumTOF_WS);
    maskdetectorsAlg->setProperty("MaskedWorkspace", constrainedWS);
    maskdetectorsAlg->executeAsSubAlg();

    sampleWS = sampleWS / vanSumTOF_WS;

    setProperty("OutputWorkspace", sampleWS);
  }


} // namespace Mantid
} // namespace Algorithms

