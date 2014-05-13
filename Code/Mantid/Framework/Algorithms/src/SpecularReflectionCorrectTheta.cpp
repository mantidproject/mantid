/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidAlgorithms/SpecularReflectionCorrectTheta.h"
#include "MantidKernel/PropertyWithValue.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace
{
  const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
  const std::string lineDetectorAnalysis = "LineDetectorAnalysis";
  const std::string pointDetectorAnalysis = "PointDetectorAnalysis";
}

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SpecularReflectionCorrectTheta)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    SpecularReflectionCorrectTheta::SpecularReflectionCorrectTheta()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    SpecularReflectionCorrectTheta::~SpecularReflectionCorrectTheta()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string SpecularReflectionCorrectTheta::name() const
    {
      return "SpecularReflectionCorrectTheta";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int SpecularReflectionCorrectTheta::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string SpecularReflectionCorrectTheta::category() const
    {
      return "Reflectometry\\ISIS";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void SpecularReflectionCorrectTheta::initDocs()
    {
      this->setWikiSummary(
          "Calculate the specular reflection two theta scattering angle (degrees) from the detector and sample locations .");
      this->setOptionalMessage(this->getWikiSummary());
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void SpecularReflectionCorrectTheta::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
          "An Input workspace to calculate the specular relection theta on.");
      this->initCommonProperties();
      declareProperty(new PropertyWithValue<double>("TwoTheta", Mantid::EMPTY_DBL(), Direction::Output),
          "Calculated two theta scattering angle in degrees.");

    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void SpecularReflectionCorrectTheta::exec()
    {
      MatrixWorkspace_sptr inWS = this->getProperty("InputWorkspace");

      const std::string analysisMode = this->getProperty("AnalysisMode");

      this->getDetectorComponent(inWS, analysisMode == pointDetectorAnalysis);
    }

  } // namespace Algorithms
} // namespace Mantid
