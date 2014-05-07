/*WIKI*

 Uses the specular reflection condition along with a supplied theta value to vertically shift the detectors into a corrected location.

 For LineDetectors and MultiDetectors, the algorithm uses an average of grouped detector locations to determine the detector position.

 *WIKI*/

#include "MantidAlgorithms/SpecularReflectionPositionCorrect.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
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

      void checkSpectrumNumbers(const std::vector<int>& spectrumNumbers, bool strictSpectrumChecking, Logger& logger)
      {
        std::set<int> uniqueSpectrumNumbers(spectrumNumbers.begin(), spectrumNumbers.end());
        if (uniqueSpectrumNumbers.size() != spectrumNumbers.size())
        {
          throw std::invalid_argument("Spectrum numbers are not unique.");
        }
        auto maxIt = std::max_element(uniqueSpectrumNumbers.begin(), uniqueSpectrumNumbers.end());
        auto minIt = std::min_element(uniqueSpectrumNumbers.begin(), uniqueSpectrumNumbers.end());
        const int max = *maxIt;
        const int min = *minIt;
        for (int i = min; i <= max; ++i)
        {
          if (uniqueSpectrumNumbers.find(i) == uniqueSpectrumNumbers.end())
          {
            const std::string message = "Spectrum numbers are not a contiguous linear sequence.";
            if(strictSpectrumChecking)
            {
              throw std::invalid_argument(message);
            }
            else
            {
              logger.warning(message);
            }
          }
        }
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
      return "Reflectometry\\ISIS";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void SpecularReflectionPositionCorrect::initDocs()
    {
      this->setWikiSummary(
          "Correct detector positions vertically based on the specular reflection condition.");
      this->setOptionalMessage(this->getWikiSummary());
    }

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

      std::vector<std::string> propOptions;
      propOptions.push_back(pointDetectorAnalysis);
      propOptions.push_back(lineDetectorAnalysis);
      propOptions.push_back(multiDetectorAnalysis);

      std::stringstream message;
      message << "The type of analysis to perform. " << multiDetectorAnalysis << ", "
          << lineDetectorAnalysis << " or " << multiDetectorAnalysis
          << ". Used to help automatically determine the detector components to move";

      declareProperty("AnalysisMode", pointDetectorAnalysis,
          boost::make_shared<StringListValidator>(propOptions), message.str());

      declareProperty(new PropertyWithValue<std::string>("DetectorComponentName", "", Direction::Input),
          "Name of the detector component i.e. point-detector. If these are not specified, the algorithm will attempt lookup using a standard naming convention.");

      declareProperty(new PropertyWithValue<std::string>("SampleComponentName", "", Direction::Input),
          "Name of the sample component i.e. some-surface-holder. If these are not specified, the algorithm will attempt lookup using a standard naming convention.");

      auto boundedArrayValidator = boost::make_shared<ArrayBoundedValidator<int> >();
      boundedArrayValidator->setLower(0);
      declareProperty(
          new ArrayProperty<int>("SpectrumNumbersOfDetectors", boundedArrayValidator, Direction::Input),
          "A list of spectrum numbers making up an effective point detector.");

      declareProperty(new PropertyWithValue<bool>("StrictSpectrumChecking", true, Direction::Input), "Enable, disable strict spectrum checking. Strict spectrum checking protects against non-sequential integers in which spectrum numbers are not in {min, min+1, ..., max}");

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "An output workspace.");

      setPropertySettings("SampleComponentName",
          new Kernel::EnabledWhenProperty("SpectrumNumbersOfGrouped", IS_NOT_DEFAULT));
      setPropertySettings("SpectrumNumbersOfDetectors",
          new Kernel::EnabledWhenProperty("SampleComponentName", IS_NOT_DEFAULT));
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
      IComponent_const_sptr detector = this->getDetectorComponent(outWS, analysisMode == pointDetectorAnalysis);
      IComponent_const_sptr sample = this->getSurfaceSampleComponent(instrument);

      correctPosition(outWS, twoThetaIn, sample, detector);

      setProperty("OutputWorkspace", outWS);
    }

    /**
     * Get the sample component. Use the name provided as a property as the basis for the lookup as a priority.
     *
     * Throws if the name is invalid.
     * @param inst : Instrument to search through
     * @return : The component : The component object found.
     */
    Mantid::Geometry::IComponent_const_sptr SpecularReflectionPositionCorrect::getSurfaceSampleComponent(
        Mantid::Geometry::Instrument_const_sptr inst)
    {
      std::string sampleComponent = "some-surface-holder";
      if (!isPropertyDefault("SampleComponentName"))
      {
        sampleComponent = this->getPropertyValue("SampleComponentName");
      }
      auto searchResult = inst->getComponentByName(sampleComponent);
      if (searchResult == NULL)
      {
        throw std::invalid_argument(sampleComponent + " does not exist. Check input properties.");
      }
      return searchResult;
    }

    /**
     * Get the detector component. Use the name provided as a property as the basis for the lookup as a priority.
     *
     * Throws if the name is invalid.
     * @param workspace : Workspace from instrument with detectors
     * @param isPointDetector : True if this is a point detector. Used to guess a name.
     * @return The component : The component object found.
     */
    boost::shared_ptr<const Mantid::Geometry::IComponent> SpecularReflectionPositionCorrect::getDetectorComponent(
        MatrixWorkspace_sptr workspace, const bool isPointDetector)
    {
      boost::shared_ptr<const IComponent> searchResult;
      if (!isPropertyDefault("SpectrumNumbersOfDetectors"))
      {
        const std::vector<int> spectrumNumbers = this->getProperty("SpectrumNumbersOfDetectors");
        const bool strictSpectrumChecking = this->getProperty("StrictSpectrumChecking");
        checkSpectrumNumbers(spectrumNumbers, strictSpectrumChecking, g_log);
        auto specToWorkspaceIndex = workspace->getSpectrumToWorkspaceIndexMap();
        DetectorGroup_sptr allDetectors = boost::make_shared<DetectorGroup>();
        bool warnIfMasked = true;
        for (size_t i = 0; i < spectrumNumbers.size(); ++i)
        {
          const size_t& spectrumNumber = spectrumNumbers[i];
          auto it = specToWorkspaceIndex.find(spectrumNumbers[i]);
          if (it == specToWorkspaceIndex.end())
          {
            std::stringstream message;
            message << "Spectrum number " << spectrumNumber << " does not exist in the InputWorkspace";
            throw std::invalid_argument(message.str());
          }
          const size_t workspaceIndex = it->second;
          auto detector = workspace->getDetector(workspaceIndex);
          allDetectors->addDetector(detector, warnIfMasked);
        }
        searchResult = allDetectors;
      }
      else
      {
        Mantid::Geometry::Instrument_const_sptr inst = workspace->getInstrument();
        std::string componentToCorrect = isPointDetector ? "point-detector" : "linedetector";

        if (!isPropertyDefault("DetectorComponentName"))
        {
          componentToCorrect = this->getPropertyValue("DetectorComponentName");

        }
        searchResult = inst->getComponentByName(componentToCorrect);
        if (searchResult == NULL)
        {
          throw std::invalid_argument(componentToCorrect + " does not exist. Check input properties.");
        }
      }

      return searchResult;
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

    /**
     * Determine if the property value is the same as the default value.
     * This can be used to determine if the property has not been set.
     * @param propertyName : Name of property to query
     * @return: True only if the property has it's default value.
     */
    bool SpecularReflectionPositionCorrect::isPropertyDefault(const std::string& propertyName) const
    {
      Property* property = this->getProperty(propertyName);
      return property->isDefault();
    }

  } // namespace Algorithms
} // namespace Mantid
