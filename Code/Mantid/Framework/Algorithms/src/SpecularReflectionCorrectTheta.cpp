/*WIKI*

Uses the Specular reflection condition ThetaIn == ThetaOut to calculate and return a corrected ThetaIn.

<math>
2*ThetaOut = tan^{-1}\frac{UpOffset}{BeamOffset}
</math>

The calculated theta value in degrees is returned by the algorithm.

Also see [[SpecularReflectionPositionCorrect]]

*WIKI*/

#include "MantidAlgorithms/SpecularReflectionCorrectTheta.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
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

      Instrument_const_sptr instrument = inWS->getInstrument();

      IComponent_const_sptr detector = this->getDetectorComponent(inWS, analysisMode == pointDetectorAnalysis);

      IComponent_const_sptr sample = this->getSurfaceSampleComponent(instrument);

      const V3D detSample  = detector->getPos() - sample->getPos();

      boost::shared_ptr<const ReferenceFrame> refFrame = instrument->getReferenceFrame();

      const double upoffset = refFrame->vecPointingUp().scalar_prod(detSample);
      const double beamoffset = refFrame->vecPointingAlongBeam().scalar_prod(detSample);

      const double twoTheta = std::atan(upoffset/beamoffset) * 180 / M_PI;

      std::stringstream strstream;
      strstream << "Recalculated two theta as: " << twoTheta;

      this->g_log.information(strstream.str());

      this->setProperty("TwoTheta", twoTheta);

    }

  } // namespace Algorithms
} // namespace Mantid
