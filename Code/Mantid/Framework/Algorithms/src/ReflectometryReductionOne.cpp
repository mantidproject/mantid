/*WIKI*
Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections. Handles both point detector and multidetector cases.
The algorithm can correct detector locations based on an input theta value.

Historically the work performed by this algorithm was known as the Quick script.

=== Analysis Modes ===

The default analysis mode is ''PointDetectorAnalysis''. Only this mode supports Transmission corrections (see below). For PointAnalysisMode
the analysis can be roughly reduced to IvsLam = DetectorWS / sum(I0) / TransmissionWS / sum(I0). The normalization by tranmission run(s) is optional.
Input workspaces are converted to ''Wavelength'' first via [[ConvertUnits]].

IvsQ is calculated via [[ConvertUnits]] into units of ''MomentumTransfer''. 
Corrections may be applied prior to the transformation to ensure that the detectors are in the correct location according to the input Theta value.
Corrections are only enabled when a Theta input value has been provided.

=== Transmission Runs ===
Transmission correction is a normalization step, which may be applied to ''PointDetectorAnalysis'' reduction.

Transmission runs are expected to be in TOF. The spectra numbers in the Transmission run workspaces must be the same as those in the Input Run workspace. 
If two Transmission runs are provided then the Stitching parameters associated with the transmission runs will also be required. 
If a single Transmission run is provided, then no stitching
parameters will be needed.

 *WIKI*/

#include "MantidAlgorithms/ReflectometryReductionOne.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include <boost/make_shared.hpp>
#include <boost/assign/list_of.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
  namespace Algorithms
  {

    /*Anonomous namespace */
    namespace
    {
      /**
       * Helper non-member function for translating all the workspace indexes in an origin workspace into workspace indexes
       * of a host end-point workspace. This is done using spectrum numbers as the intermediate.
       *
       * This function will throw a runtime error if the specId are not found to exist on the host end-point workspace.
       *
       * @param originWS : Origin workspace, which provides the original workspace index to spectrum id mapping.
       * @param hostWS : Workspace onto which the resulting workspace indexes will be hosted
       * @return Remapped wokspace indexes applicable for the host workspace. results as comma separated string.
       */
      std::string createWorkspaceIndexListFromDetectorWorkspace(MatrixWorkspace_const_sptr originWS,
          MatrixWorkspace_const_sptr hostWS)
      {
        auto spectrumMap = originWS->getSpectrumToWorkspaceIndexMap();
        auto it = spectrumMap.begin();
        std::stringstream result;
        specid_t specId = (*it).first;
        result << static_cast<int>(hostWS->getIndexFromSpectrumNumber(specId));
        ++it;
        for (; it != spectrumMap.end(); ++it)
        {
          specId = (*it).first;
          result << "," << static_cast<int>(hostWS->getIndexFromSpectrumNumber(specId));
        }
        return result.str();
      }

      const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
      const std::string pointDetectorAnalysis = "PointDetectorAnalysis";
      const std::string tofUnitId = "TOF";
      const std::string wavelengthUnitId = "Wavelength";
    }
    /* End of ananomous namespace */

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(ReflectometryReductionOne)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    ReflectometryReductionOne::ReflectometryReductionOne()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    ReflectometryReductionOne::~ReflectometryReductionOne()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string ReflectometryReductionOne::name() const
    {
      return "ReflectometryReductionOne";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int ReflectometryReductionOne::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string ReflectometryReductionOne::category() const
    {
      return "Reflectometry\\ISIS";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void ReflectometryReductionOne::initDocs()
    {
      this->setOptionalMessage("Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections.");
      this->setWikiSummary("Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections. See [[Reflectometry_Guide]]");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void ReflectometryReductionOne::init()
    {
      boost::shared_ptr<CompositeValidator> inputValidator = boost::make_shared<CompositeValidator>();
      inputValidator->add(boost::make_shared<WorkspaceUnitValidator>(tofUnitId));

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input, inputValidator),
          "Run to reduce.");

      std::vector<std::string> propOptions;
      propOptions.push_back(pointDetectorAnalysis);
      propOptions.push_back(multiDetectorAnalysis);

      declareProperty("AnalysisMode", "PointDetectorAnalysis",
          boost::make_shared<StringListValidator>(propOptions),
          "The type of analysis to perform. Point detector or multi detector.");

      declareProperty(new ArrayProperty<int>("RegionOfInterest"),
          "Indices of the spectra a pair (lower, upper) that mark the ranges that correspond to the region of interest (reflected beam) in multi-detector mode.");
      declareProperty(new ArrayProperty<int>("RegionOfDirectBeam"),
          "Indices of the spectra a pair (lower, upper) that mark the ranges that correspond to the direct beam in multi-detector mode.");

      this->initIndexInputs();
      this->initWavelengthInputs();

      declareProperty(new PropertyWithValue<std::string>("DetectorComponentName", "", Direction::Input),
          "Name of the detector component i.e. point-detector. If these are not specified, the algorithm will attempt lookup using a standard naming convention.");

      declareProperty(new PropertyWithValue<std::string>("SampleComponentName", "", Direction::Input),
          "Name of the sample component i.e. some-surface-holder. If these are not specified, the algorithm will attempt lookup using a standard naming convention.");

      declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
          "Output Workspace IvsQ.");

      declareProperty(
          new WorkspaceProperty<>("OutputWorkspaceWavelength", "", Direction::Output,
              PropertyMode::Optional), "Output Workspace IvsLam. Intermediate workspace.");

      declareProperty(new PropertyWithValue<double>("ThetaIn", Mantid::EMPTY_DBL(), Direction::Input),
          "Final theta value in degrees. Optional, this value will be calculated internally and provided as ThetaOut if not provided.");

      declareProperty(new PropertyWithValue<double>("ThetaOut", Mantid::EMPTY_DBL(), Direction::Output),
          "Calculated final theta in degrees.");

      declareProperty(new PropertyWithValue<bool>("CorrectDetectorPositions", true, Direction::Input),
          "Correct detector positions using ThetaIn (if given)");

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("FirstTransmissionRun", "", Direction::Input,
              PropertyMode::Optional),
          "First transmission run, or the low wavelength transmision run if SecondTransmissionRun is also provided.");
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionRun", "", Direction::Input,
              PropertyMode::Optional, inputValidator->clone()),
          "Second, high wavelength transmission run. Optional. Causes the FirstTransmissionRun to be treated as the low wavelength transmission run.");

      this->initStitchingInputs();

      declareProperty(
        new PropertyWithValue<bool>("StrictSpectrumChecking", true, Direction::Input),
        "Enforces spectrum number checking prior to normalisation");

      setPropertyGroup("FirstTransmissionRun", "Transmission");
      setPropertyGroup("SecondTransmissionRun", "Transmission");
      setPropertyGroup("Params", "Transmission");
      setPropertyGroup("StartOverlap", "Transmission");
      setPropertyGroup("EndOverlap", "Transmission");

      // Only do transmission corrections when point detector.
      setPropertySettings("FirstTransmissionRun",
          new Kernel::EnabledWhenProperty("AnalysisMode", IS_EQUAL_TO, "PointDetectorAnalysis"));
      setPropertySettings("SecondTransmissionRun",
          new Kernel::EnabledWhenProperty("AnalysisMode", IS_EQUAL_TO, "PointDetectorAnalysis"));
      setPropertySettings("Params",
          new Kernel::EnabledWhenProperty("AnalysisMode", IS_EQUAL_TO, "PointDetectorAnalysis"));
      setPropertySettings("StartOverlap",
          new Kernel::EnabledWhenProperty("AnalysisMode", IS_EQUAL_TO, "PointDetectorAnalysis"));
      setPropertySettings("EndOverlap",
          new Kernel::EnabledWhenProperty("AnalysisMode", IS_EQUAL_TO, "PointDetectorAnalysis"));

      // Only use region of direct beam when in multi-detector analysis mode.
      setPropertySettings("RegionOfDirectBeam",
          new Kernel::EnabledWhenProperty("AnalysisMode", IS_EQUAL_TO, "MultiDetectorAnalysis"));
    }

    /**
     * Correct the position of the detectors based on the input theta value.
     * @param toCorrect : Workspace to correct detector posisitions on.
     * @param thetaInDeg : Theta in degrees to use in correction calculations.
     * @param sample : Pointer to the sample
     * @param detector : Pointer to a given detector
     */
    void ReflectometryReductionOne::correctPosition(API::MatrixWorkspace_sptr toCorrect,
        const double& thetaInDeg, IComponent_const_sptr sample, IComponent_const_sptr detector)
    {

      auto instrument = toCorrect->getInstrument();

      const V3D detectorPosition = detector->getPos();

      const V3D samplePosition = sample->getPos();

      const V3D sampleToDetector = detectorPosition - samplePosition;

      auto referenceFrame = instrument->getReferenceFrame();

      const double sampleToDetectorAlongBeam = sampleToDetector.scalar_prod(
          referenceFrame->vecPointingAlongBeam());

      const double thetaInRad = thetaInDeg * (M_PI / 180.0);

      double acrossOffset = 0;

      double beamOffset = detectorPosition.scalar_prod(referenceFrame->vecPointingAlongBeam());

      double upOffset = sampleToDetectorAlongBeam * std::sin(2.0 * thetaInRad);

      auto moveComponentAlg = this->createChildAlgorithm("MoveInstrumentComponent");
      moveComponentAlg->initialize();
      moveComponentAlg->setProperty("Workspace", toCorrect);
      moveComponentAlg->setProperty("ComponentName", detector->getName());
      moveComponentAlg->setProperty("RelativePosition", false);
      // Movements
      moveComponentAlg->setProperty(referenceFrame->pointingAlongBeamAxis(), beamOffset);
      moveComponentAlg->setProperty(referenceFrame->pointingHorizontalAxis(), acrossOffset);
      moveComponentAlg->setProperty(referenceFrame->pointingUpAxis(), upOffset);
      // Execute the movement.
      moveComponentAlg->execute();

    }

    /**
     * Convert an input workspace into an IvsQ workspace.
     *
     * @param toConvert : Workspace to convert
     * @param bCorrectPosition : Flag to indicate that detector positions should be corrected based on the input theta values.
     * @param thetaInDeg : Theta in Degrees. Used for correction.
     * @param isPointDetector: Is point detector analysis
     * @return
     */
    Mantid::API::MatrixWorkspace_sptr ReflectometryReductionOne::toIvsQ(
        API::MatrixWorkspace_sptr toConvert, const bool bCorrectPosition, OptionalDouble& thetaInDeg, const bool isPointDetector)
    {
      /*
       * Can either calculate a missing theta value for the purposes of reporting, or correct positions based on a theta value,
       * but not both. The processing is effectively circular if both are applied.
       */
      if (!thetaInDeg.is_initialized())
      {
        g_log.debug("Calculating final theta.");
        const double thetaToRad = 180 / M_PI;

        Instrument_const_sptr instrument = toConvert->getInstrument();

        IComponent_const_sptr detector = this->getDetectorComponent(instrument, isPointDetector);
        IComponent_const_sptr sample = this->getSurfaceSampleComponent(instrument);

        const V3D sampleToDetectorPos = detector->getPos() - sample->getPos();

        const V3D sourcePos = instrument->getSource()->getPos();
        const V3D beamPos = sample->getPos() - sourcePos;

        const V3D sampleDetVec = detector->getPos() - sample->getPos();
        const double calculatedTheta = sampleDetVec.angle(beamPos) * thetaToRad * 1 / 2;

        thetaInDeg = calculatedTheta / thetaToRad; // Assign calculated value it.
      }
      else if (bCorrectPosition) // This probably ought to be an automatic decision. How about making a guess about sample position holder and detector names. But also allowing the two component names (sample and detector) to be passed in.
      {
        g_log.debug("Correcting detector position");

        auto instrument = toConvert->getInstrument();
        IComponent_const_sptr detector = this->getDetectorComponent(instrument, isPointDetector);
        IComponent_const_sptr sample = this->getSurfaceSampleComponent(instrument);

        correctPosition(toConvert, thetaInDeg.get(), sample, detector);
      }

      // Always convert units.
      auto convertUnits = this->createChildAlgorithm("ConvertUnits");
      convertUnits->initialize();
      convertUnits->setProperty("InputWorkspace", toConvert);
      convertUnits->setProperty("Target", "MomentumTransfer");
      convertUnits->execute();
      MatrixWorkspace_sptr inQ = convertUnits->getProperty("OutputWorkspace");
      return inQ;
    }

    /**
     * Get the sample component. Use the name provided as a property as the basis for the lookup as a priority.
     *
     * Throws if the name is invalid.
     * @param inst : Instrument to search through
     * @return : The component : The component object found.
     */
    Mantid::Geometry::IComponent_const_sptr ReflectometryReductionOne::getSurfaceSampleComponent(
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
     * @param inst : Instrument to search through.
     * @param isPointDetector : True if this is a point detector. Used to guess a name.
     * @return The component : The component object found.
     */
    boost::shared_ptr<const Mantid::Geometry::IComponent> ReflectometryReductionOne::getDetectorComponent(
        Mantid::Geometry::Instrument_const_sptr inst, const bool isPointDetector)
    {
      std::string componentToCorrect = isPointDetector ? "point-detector" : "line-detector";
      if (!isPropertyDefault("DetectorComponentName"))
      {
        componentToCorrect = this->getPropertyValue("DetectorComponentName");
      }
      boost::shared_ptr<const IComponent> searchResult = inst->getComponentByName(componentToCorrect);
      if (searchResult == NULL)
      {
        throw std::invalid_argument(componentToCorrect + " does not exist. Check input properties.");
      }
      return searchResult;
    }

    /**
     * Sum spectra over a specified range.
     * @param inWS
     * @param startIndex
     * @param endIndex
     * @return Workspace with spectra summed over the specified range.
     */
    MatrixWorkspace_sptr ReflectometryReductionOne::sumSpectraOverRange(MatrixWorkspace_sptr inWS,
        const int startIndex, const int endIndex)
    {
      auto sumSpectra = this->createChildAlgorithm("SumSpectra");
      sumSpectra->initialize();
      sumSpectra->setProperty("InputWorkspace", inWS);
      sumSpectra->setProperty("StartWorkspaceIndex", startIndex);
      sumSpectra->setProperty("EndWorkspaceIndex", endIndex);
      sumSpectra->execute();
      MatrixWorkspace_sptr outWS = sumSpectra->getProperty("OutputWorkspace");
      return outWS;
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void ReflectometryReductionOne::exec()
    {
      MatrixWorkspace_sptr runWS = getProperty("InputWorkspace");

      OptionalMatrixWorkspace_sptr firstTransmissionRun;
      OptionalMatrixWorkspace_sptr secondTransmissionRun;
      OptionalDouble stitchingStart;
      OptionalDouble stitchingDelta;
      OptionalDouble stitchingEnd;
      OptionalDouble stitchingStartOverlap;
      OptionalDouble stitchingEndOverlap;

      getTransmissionRunInfo(firstTransmissionRun, secondTransmissionRun, stitchingStart, stitchingDelta,
          stitchingEnd, stitchingStartOverlap, stitchingEndOverlap);

      OptionalDouble theta;
      if (!isPropertyDefault("ThetaIn"))
      {
        double temp = this->getProperty("ThetaIn");
        theta = temp;
      }

      const std::string strAnalysisMode = getProperty("AnalysisMode");
      const bool isPointDetector = (pointDetectorAnalysis.compare(strAnalysisMode) == 0);
      const bool isMultiDetector = (multiDetectorAnalysis.compare(strAnalysisMode) == 0);

      const MinMax wavelengthInterval = this->getMinMax("WavelengthMin", "WavelengthMax");
      const double wavelengthStep = getProperty("WavelengthStep");
      const MinMax monitorBackgroundWavelengthInterval = getMinMax("MonitorBackgroundWavelengthMin",
          "MonitorBackgroundWavelengthMax");
      const MinMax monitorIntegrationWavelengthInterval = getMinMax("MonitorIntegrationWavelengthMin",
          "MonitorIntegrationWavelengthMax");

      const std::string processingCommands = getWorkspaceIndexList();

      OptionalWorkspaceIndexes directBeam;
      fetchOptionalLowerUpperPropertyValue("RegionOfDirectBeam", isPointDetector, directBeam);

      const int i0MonitorIndex = getProperty("I0MonitorIndex");

      const bool correctDetctorPositions = getProperty("CorrectDetectorPositions");

      DetectorMonitorWorkspacePair inLam = toLam(runWS, processingCommands, i0MonitorIndex,
          wavelengthInterval, monitorBackgroundWavelengthInterval, wavelengthStep);
      auto detectorWS = inLam.get<0>();
      auto monitorWS = inLam.get<1>();

      MatrixWorkspace_sptr IvsLam; // Output workspace
      MatrixWorkspace_sptr IvsQ; // Output workspace
      if (isMultiDetector)
      {
        if (directBeam.is_initialized())
        {
          // Sum over the direct beam.
          WorkspaceIndexList db = directBeam.get();
          std::stringstream buffer;
          buffer << db.front() << "-" << db.back();
          MatrixWorkspace_sptr regionOfDirectBeamWS = this->toLamDetector(buffer.str(), runWS,
              wavelengthInterval, wavelengthStep);

          // Rebin to the detector workspace
          auto rebinToWorkspaceAlg = this->createChildAlgorithm("RebinToWorkspace");
          rebinToWorkspaceAlg->initialize();
          rebinToWorkspaceAlg->setProperty("WorkspaceToRebin", regionOfDirectBeamWS);
          rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", detectorWS);
          rebinToWorkspaceAlg->execute();
          regionOfDirectBeamWS = rebinToWorkspaceAlg->getProperty("OutputWorkspace");

          // Normalize by the direct beam.
          detectorWS = detectorWS / regionOfDirectBeamWS;
        }
      }
      auto integrationAlg = this->createChildAlgorithm("Integration");
      integrationAlg->initialize();
      integrationAlg->setProperty("InputWorkspace", monitorWS);
      integrationAlg->setProperty("RangeLower", monitorIntegrationWavelengthInterval.get<0>());
      integrationAlg->setProperty("RangeUpper", monitorIntegrationWavelengthInterval.get<1>());
      integrationAlg->execute();
      MatrixWorkspace_sptr integratedMonitor = integrationAlg->getProperty("OutputWorkspace");

      IvsLam = detectorWS / integratedMonitor; // Normalize by the integrated monitor counts.

      if (firstTransmissionRun.is_initialized())
      {

        // Perform transmission correction.
        IvsLam = this->transmissonCorrection(IvsLam, wavelengthInterval,
            monitorBackgroundWavelengthInterval, monitorIntegrationWavelengthInterval, i0MonitorIndex,
            firstTransmissionRun.get(), secondTransmissionRun, stitchingStart, stitchingDelta,
            stitchingEnd, stitchingStartOverlap, stitchingEndOverlap, wavelengthStep, processingCommands);
      }
      else
      {
        g_log.warning("No transmission correction will be applied.");
      }

      IvsQ = this->toIvsQ(IvsLam, correctDetctorPositions, theta, isPointDetector);

      setProperty("ThetaOut", theta.get());
      setProperty("OutputWorkspaceWavelength", IvsLam);
      setProperty("OutputWorkspace", IvsQ);

    }

    /**
     * Perform Transmission Corrections.
     * @param IvsLam : Run workspace which is to be normalized by the results of the transmission corrections.
     * @param wavelengthInterval : Wavelength interval for the run workspace.
     * @param wavelengthMonitorBackgroundInterval : Wavelength interval for the monitor background
     * @param wavelengthMonitorIntegrationInterval : Wavelength interval for the monitor integration
     * @param i0MonitorIndex : Monitor index for the I0 monitor
     * @param firstTransmissionRun : The first transmission run
     * @param secondTransmissionRun : The second transmission run (optional)
     * @param stitchingStart : Stitching start in wavelength (optional but dependent on secondTransmissionRun)
     * @param stitchingDelta : Stitching delta in wavelength (optional but dependent on secondTransmissionRun)
     * @param stitchingEnd : Stitching end in wavelength (optional but dependent on secondTransmissionRun)
     * @param stitchingStartOverlap : Stitching start wavelength overlap (optional but dependent on secondTransmissionRun)
     * @param stitchingEndOverlap : Stitching end wavelength overlap (optional but dependent on secondTransmissionRun)
     * @param wavelengthStep : Step in angstroms for rebinning for workspaces converted into wavelength.
     * @param numeratorProcessingCommands: Processing commands used on detector workspace.
     * @return Normalized run workspace by the transmission workspace, which have themselves been converted to Lam, normalized by monitors and possibly stitched together.
     */
    MatrixWorkspace_sptr ReflectometryReductionOne::transmissonCorrection(MatrixWorkspace_sptr IvsLam,
        const MinMax& wavelengthInterval, const MinMax& wavelengthMonitorBackgroundInterval,
        const MinMax& wavelengthMonitorIntegrationInterval, const int& i0MonitorIndex,
        MatrixWorkspace_sptr firstTransmissionRun, OptionalMatrixWorkspace_sptr secondTransmissionRun,
        const OptionalDouble& stitchingStart, const OptionalDouble& stitchingDelta,
        const OptionalDouble& stitchingEnd, const OptionalDouble& stitchingStartOverlap,
        const OptionalDouble& stitchingEndOverlap, const double& wavelengthStep, const std::string& numeratorProcessingCommands)
    {
      g_log.debug("Extracting first transmission run workspace indexes from spectra");

      const bool strictSpectrumChecking = getProperty("StrictSpectrumChecking");
    
      MatrixWorkspace_sptr denominator = firstTransmissionRun;
      Unit_const_sptr xUnit = firstTransmissionRun->getAxis(0)->unit();
      if (xUnit->unitID() == tofUnitId)
      {
        std::string spectrumProcessingCommands = numeratorProcessingCommands;
        /*
        If we have strict spectrum checking, the processing commands need to be made from the 
        numerator workspace AND the transmission workspace based on matching spectrum numbers.
        */
        if(strictSpectrumChecking)
        {
          spectrumProcessingCommands = createWorkspaceIndexListFromDetectorWorkspace(IvsLam,
          firstTransmissionRun);
        }

        // Make the transmission run.
        auto alg = this->createChildAlgorithm("CreateTransmissionWorkspace");
        alg->initialize();
        alg->setProperty("FirstTransmissionRun", firstTransmissionRun);
        if (secondTransmissionRun.is_initialized())
        {
          alg->setProperty("SecondTransmissionRun", secondTransmissionRun.get());
          const std::vector<double> params = boost::assign::list_of(stitchingStart.get())(
              stitchingDelta.get())(stitchingEnd.get()).convert_to_container<std::vector<double> >();
          alg->setProperty("Params", params);
          alg->setProperty("StartOverlap", stitchingStartOverlap.get());
          alg->setProperty("EndOverlap", stitchingEndOverlap.get());
        }
        alg->setProperty("ProcessingInstructions", spectrumProcessingCommands);
        alg->setProperty("I0MonitorIndex", i0MonitorIndex);
        alg->setProperty("WavelengthMin", wavelengthInterval.get<0>());
        alg->setProperty("WavelengthMax", wavelengthInterval.get<1>());
        alg->setProperty("WavelengthStep", wavelengthStep);
        alg->setProperty("MonitorBackgroundWavelengthMin", wavelengthMonitorBackgroundInterval.get<0>());
        alg->setProperty("MonitorBackgroundWavelengthMax", wavelengthMonitorBackgroundInterval.get<1>());
        alg->setProperty("MonitorIntegrationWavelengthMin",
            wavelengthMonitorIntegrationInterval.get<0>());
        alg->setProperty("MonitorIntegrationWavelengthMax",
            wavelengthMonitorIntegrationInterval.get<1>());
        alg->execute();
        denominator = alg->getProperty("OutputWorkspace");
      }

      // Rebin the transmission run to be the same as the input.
      auto rebinToWorkspaceAlg = this->createChildAlgorithm("RebinToWorkspace");
      rebinToWorkspaceAlg->initialize();
      rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", IvsLam);
      rebinToWorkspaceAlg->setProperty("WorkspaceToRebin", denominator);
      rebinToWorkspaceAlg->execute();
      denominator = rebinToWorkspaceAlg->getProperty("OutputWorkspace");

      verifySpectrumMaps(IvsLam, denominator, strictSpectrumChecking);

      // Do normalization.
      MatrixWorkspace_sptr normalizedIvsLam = IvsLam / denominator;
      return normalizedIvsLam;
    }

    /**
    @param ws1 : First workspace to compare
    @param ws2 : Second workspace to compare against
    @param severe: True to indicate that failure to verify should result in an exception. Otherwise a warning is generated.
    */
    void ReflectometryReductionOne::verifySpectrumMaps(MatrixWorkspace_const_sptr ws1, MatrixWorkspace_const_sptr ws2, const bool severe)
    {
      auto map1 = ws1->getSpectrumToWorkspaceIndexMap();
      auto map2 = ws2->getSpectrumToWorkspaceIndexMap();
      if (map1 != map2)
      {
        std::string message = "Spectrum maps between workspaces do NOT match up.";
        if (severe)
        {
          throw std::invalid_argument(message);
        }
        else
        {
          this->g_log.warning(message);
        }
      }
    }

  } // namespace Algorithms
} // namespace Mantid
