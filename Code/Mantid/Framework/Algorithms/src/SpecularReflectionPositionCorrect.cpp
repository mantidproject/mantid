#include "MantidAlgorithms/SpecularReflectionPositionCorrect.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    namespace
    {
      const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
      const std::string lineDetectorAnalysis = "LineDetectorAnalysis";
      const std::string pointDetectorAnalysis = "PointDetectorAnalysis";

      /**
       * Get the root component, that is not the instrument itself.
       * @param currentComponent : Some component in the tree
       * @return : Parent component.
       */
      IComponent_const_sptr getRootComponent(IComponent_const_sptr& currentComponent)
      {
        if (IComponent_const_sptr parent = currentComponent->getParent())
        {
          if (!dynamic_cast<Instrument*>(const_cast<IComponent*>(parent.get())))
          {
            return getRootComponent(parent);
          }
        }
        return currentComponent;
      }

      /**
       * Determine if there is a common parent component.
       * @param detectors : Detectors to evaluate.
       * @return True only if all detectors have the same immediate parent.
       */
      bool hasCommonParent(const std::vector<IDetector_const_sptr>& detectors)
      {
        bool sameParentComponent = true;
        IComponent const * lastParentComponent = detectors[0]->getParent().get();
        for (size_t i = 1; i < detectors.size(); ++i)
        {
          IComponent const * currentParentComponent = detectors[i]->getParent().get();
          if (lastParentComponent != currentParentComponent)
          {
            sameParentComponent = false; // Parent components are different.
            break;
          }
          lastParentComponent = currentParentComponent;
        }
        return sameParentComponent;
      }

    }
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SpecularReflectionPositionCorrect)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    SpecularReflectionPositionCorrect::SpecularReflectionPositionCorrect()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    SpecularReflectionPositionCorrect::~SpecularReflectionPositionCorrect()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string SpecularReflectionPositionCorrect::name() const
    {
      return "SpecularReflectionPositionCorrect";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int SpecularReflectionPositionCorrect::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string SpecularReflectionPositionCorrect::category() const
    {
      return "Reflectometry";
    }

    //----------------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void SpecularReflectionPositionCorrect::init()
    {
      auto thetaValidator = boost::make_shared<CompositeValidator>();
      thetaValidator->add(boost::make_shared<MandatoryValidator<double> >());
      thetaValidator->add(boost::make_shared<BoundedValidator<double> >(0, 90, true));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
          "An input workspace to correct.");

      declareProperty(
          new PropertyWithValue<double>("TwoThetaIn", Mantid::EMPTY_DBL(), thetaValidator,
              Direction::Input), "Input two theta angle in degrees.");

      this->initCommonProperties();

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "An output workspace.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void SpecularReflectionPositionCorrect::exec()
    {

      MatrixWorkspace_sptr inWS = this->getProperty("InputWorkspace");
      const std::string analysisMode = this->getProperty("AnalysisMode");
      auto cloneWS = this->createChildAlgorithm("CloneWorkspace");
      cloneWS->initialize();
      cloneWS->setProperty("InputWorkspace", inWS);
      cloneWS->execute();
      Workspace_sptr tmp = cloneWS->getProperty("OutputWorkspace");
      MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tmp);

      const double twoThetaIn = this->getProperty("TwoThetaIn");

      auto instrument = outWS->getInstrument();
      IComponent_const_sptr detector = this->getDetectorComponent(outWS,
          analysisMode == pointDetectorAnalysis);
      IComponent_const_sptr sample = this->getSurfaceSampleComponent(instrument);

      correctPosition(outWS, twoThetaIn, sample, detector);

      setProperty("OutputWorkspace", outWS);
    }

    /**
     * Execute the MoveInstrumentComponent on all (named) subcomponents
     * @param toCorrect : Workspace to correct
     * @param detector : Detector or DetectorGroup
     * @param sample : Sample Component
     * @param upOffset : Up offset to apply
     * @param acrossOffset : Across offset to apply
     * @param detectorPosition: Actual detector or detector group position.
     */
    void SpecularReflectionPositionCorrect::moveDetectors(API::MatrixWorkspace_sptr toCorrect,
        IComponent_const_sptr detector, IComponent_const_sptr sample, const double& upOffset,
        const double& acrossOffset, const V3D& detectorPosition)
    {
      auto instrument = toCorrect->getInstrument();
      const V3D samplePosition = sample->getPos();
      auto referenceFrame = instrument->getReferenceFrame();
      if (auto groupDetector = boost::dynamic_pointer_cast<const DetectorGroup>(detector)) // Do we have a group of detectors
      {
        const std::vector<IDetector_const_sptr> detectors = groupDetector->getDetectors();
        const bool commonParent = hasCommonParent(detectors);
        if (commonParent)
        {
          /*
           * Same parent component. So lets move that.
           */
          moveDetectors(toCorrect, detectors[0], sample, upOffset, acrossOffset, detectorPosition); // Recursive call
        }
        else
        {
          /*
           * We have to move individual components.
           */
          for (size_t i = 0; i < detectors.size(); ++i)
          {
            moveDetectors(toCorrect, detectors[i], sample, upOffset, acrossOffset, detectorPosition); // Recursive call
          }
        }
      }
      else
      {
        auto moveComponentAlg = this->createChildAlgorithm("MoveInstrumentComponent");
        moveComponentAlg->initialize();
        moveComponentAlg->setProperty("Workspace", toCorrect);
        IComponent_const_sptr root = getRootComponent(detector);
        const std::string componentName = root->getName();
        moveComponentAlg->setProperty("ComponentName", componentName);
        moveComponentAlg->setProperty("RelativePosition", false);
        // Movements
        moveComponentAlg->setProperty(referenceFrame->pointingAlongBeamAxis(),
            detectorPosition.scalar_prod(referenceFrame->vecPointingAlongBeam()));
        moveComponentAlg->setProperty(referenceFrame->pointingHorizontalAxis(), acrossOffset);
        const double detectorVerticalPosition = detectorPosition.scalar_prod(
            referenceFrame->vecPointingUp());
        const double rootVerticalPosition = root->getPos().scalar_prod(referenceFrame->vecPointingUp());

        const double dm = rootVerticalPosition - detectorVerticalPosition;
        moveComponentAlg->setProperty(referenceFrame->pointingUpAxis(),
            samplePosition.scalar_prod(referenceFrame->vecPointingUp()) + upOffset + dm);
        // Execute the movement.
        moveComponentAlg->execute();

      }
    }

    /**
     * Correct the position of the detectors based on the input theta value.
     * @param toCorrect : Workspace to correct detector posisitions on.
     * @param twoThetaInDeg : 2* Theta in degrees to use in correction calculations.
     * @param sample : Pointer to the sample
     * @param detector : Pointer to a given detector
     */
    void SpecularReflectionPositionCorrect::correctPosition(API::MatrixWorkspace_sptr toCorrect,
        const double& twoThetaInDeg, IComponent_const_sptr sample, IComponent_const_sptr detector)
    {

      auto instrument = toCorrect->getInstrument();

      const V3D detectorPosition = detector->getPos();

      const V3D samplePosition = sample->getPos();

      const V3D sampleToDetector = detectorPosition - samplePosition;

      auto referenceFrame = instrument->getReferenceFrame();

      const double twoThetaInRad = twoThetaInDeg * (M_PI / 180.0);

      double acrossOffset = 0;

      double beamOffset = sampleToDetector.scalar_prod(referenceFrame->vecPointingAlongBeam()); // We just recalculate beam offset.

      double upOffset = (beamOffset * std::tan(twoThetaInRad)); // We only correct vertical position

      // Apply the movements.
      moveDetectors(toCorrect, detector, sample, upOffset, acrossOffset, detector->getPos());

    }

  } // namespace Algorithms
} // namespace Mantid
