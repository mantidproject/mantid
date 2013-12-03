/*WIKI*
 Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections. Handles both point detector and multidetector cases.
 *WIKI*/

#include "MantidAlgorithms/ReflectometryReductionOne.h"
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

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
  namespace Algorithms
  {

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
      this->setWikiSummary(
          "Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections.");
      this->setOptionalMessage(this->getWikiSummary());
    }

    namespace
    {
      const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
      const std::string pointDetectorAnalysis = "PointDetectorAnalysis";
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void ReflectometryReductionOne::init()
    {
      boost::shared_ptr<CompositeValidator> inputValidator = boost::make_shared<CompositeValidator>();
      inputValidator->add(boost::make_shared<WorkspaceUnitValidator>("TOF"));

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input, inputValidator),
          "Run to reduce.");
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("FirstTransmissionRun", "", Direction::Input,
              PropertyMode::Optional, inputValidator->clone()),
          "First transmission run, or the low wavelength transmision run if SecondTransmissionRun is also provided.");
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionRun", "", Direction::Input,
              PropertyMode::Optional, inputValidator->clone()),
          "Second, high wavelength transmission run. Optional. Causes the FirstTransmissionRun to be treated as the low wavelength transmission run.");
      declareProperty(new PropertyWithValue<double>("Theta", -1, Direction::Input),
          "Final theta value.");

      declareProperty(
          new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
          "A comma separated list of first bin boundary, width, last bin boundary. "
              "These parameters are used for stitching together transmission runs. "
              "Values are in q. This input is only needed if a SecondTransmission run is provided.");

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

      declareProperty(
          new PropertyWithValue<double>("WavelengthMin", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength minimum");
      declareProperty(
          new PropertyWithValue<double>("WavelengthMax", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength maximum");

      boost::shared_ptr<CompositeValidator> mandatoryWorkspaceIndex = boost::make_shared<
          CompositeValidator>();
      mandatoryWorkspaceIndex->add(boost::make_shared<MandatoryValidator<int> >());
      auto boundedIndex = boost::make_shared<BoundedValidator<int> >();
      boundedIndex->setLower(0);
      mandatoryWorkspaceIndex->add(boundedIndex);

      declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
          "Indices of the spectra in pairs (lower, upper) that mark the ranges that correspond to detectors of interest.");

      declareProperty(
          new PropertyWithValue<int>("I0MonitorIndex", Mantid::EMPTY_INT(), mandatoryWorkspaceIndex),
          "I0 monitor index");

      declareProperty(
          new PropertyWithValue<double>("MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength minimum for monitor background. Taken to be WavelengthMin if not provided.");

      declareProperty(
          new PropertyWithValue<double>("MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength maximum for monitor background. Taken to be WavelengthMax if not provided.");

      declareProperty(
          new PropertyWithValue<double>("MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength minimum for integration. Taken to be WavelengthMin if not provided.");
      declareProperty(
          new PropertyWithValue<double>("MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength maximum for integration. Taken to be WavelengthMax if not provided.");

      declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));
    }

    bool ReflectometryReductionOne::isPropertyDefault(const std::string& propertyName) const
    {
      Property* property = this->getProperty(propertyName);
      return property->isDefault();
    }

    bool checkNotPositive(const int value)
    {
      return value < 0;
    }

    /**
     * Get the workspace index list
     * @return Workspace index list.
     */
    ReflectometryReductionOne::WorkspaceIndexList ReflectometryReductionOne::getWorkspaceIndexList()
    {
      WorkspaceIndexList indexList = getProperty("WorkspaceIndexList");
      if (indexList.size() % 2 != 0 || indexList.size() == 0)
      {
        throw std::invalid_argument(
            "WorkspaceIndex list must be composed of pairs of min index, max index.");
      }

      if (std::find_if(indexList.begin(), indexList.end(), checkNotPositive) != indexList.end())
      {
        throw std::invalid_argument("WorkspaceIndexList contains negative indexes");
      }

      for (size_t i = 0; (i + 1) < indexList.size(); i += 2)
      {
        if (indexList[i] > indexList[i + 1])
          throw std::invalid_argument("WorkspaceIndexList pairs must be in min, max order");
      }
      return indexList;
    }

    /**
     * Fetch min, max inputs as a vector (int) if they are non-default and set them to the optionalUpperLower object.
     * Performs checks to verify that invalid indexes have not been passed in.
     * @param propertyName : Property name to fetch
     * @param isPointDetector : Flag indicates that the execution is in point detector mode.
     * @param optionalUpperLower : Object to set min and max on.
     */
    void ReflectometryReductionOne::fetchOptionalLowerUpperPropertyValue(const std::string& propertyName,
        bool isPointDetector, OptionalWorkspaceIndexes& optionalUpperLower)
    {
      if (!isPropertyDefault(propertyName))
      {
        // Validation of property inputs.
        if (isPointDetector)
        {
          throw std::invalid_argument(
              "Cannot have a region of interest property in point detector mode.");
        }
        std::vector<int> temp = this->getProperty(propertyName);
        if (temp.size() != 2)
        {
          const std::string message = propertyName + " requires a lower and upper boundary";
          throw std::invalid_argument(message);
        }
        if (temp[0] > temp[1])
        {
          throw std::invalid_argument("Min must be <= Max index");
        }
        if (std::find_if(temp.begin(), temp.end(), checkNotPositive) != temp.end())
        {
          const std::string message = propertyName + " contains negative indexes";
          throw std::invalid_argument(message);
        }
        // Assignment
        optionalUpperLower = temp;
      }
    }

    /**
     * Get min max pairs as a tuple.
     * @param minProperty : Property name for the min property
     * @param maxProperty : Property name for the max property
     * @return A tuple consisting of min, max
     */
    ReflectometryReductionOne::MinMax ReflectometryReductionOne::getMinMax(
        const std::string& minProperty, const std::string& maxProperty)
    {
      const double min = getProperty(minProperty);
      const double max = getProperty(maxProperty);
      if (min > max)
      {
        throw std::invalid_argument("Cannot have any WavelengthMin > WavelengthMax");
      }
      return MinMax(min, max);
    }

    /**
     * Get the transmission run information.
     *
     * Transmission runs are optional, but you cannot have the second without the first. Also, stitching
     * parameters are required if the second is present. This getter fetches and assigns to the optional reference arguments
     *
     * @param firstTransmissionRun
     * @param secondTransmissionRun
     * @param stitchingStartQ
     * @param stitchingDeltaQ
     * @param stitchingEndQ
     */
    void ReflectometryReductionOne::getTransmissionRunInfo(
        OptionalMatrixWorkspace_sptr firstTransmissionRun,
        OptionalMatrixWorkspace_sptr secondTransmissionRun, OptionalDouble& stitchingStartQ,
        OptionalDouble& stitchingDeltaQ, OptionalDouble& stitchingEndQ)
    {
      if (!isPropertyDefault("FirstTransmissionRun"))
      {
        MatrixWorkspace_sptr temp = this->getProperty("FirstTransmissionRun");
        firstTransmissionRun = temp;
      }

      if (!isPropertyDefault("SecondTransmissionRun"))
      {
        if (isPropertyDefault("FirstTransmissionRun"))
        {
          throw std::invalid_argument(
              "A SecondTransmissionRun is only valid if a FirstTransmissionRun is provided.");
        }
        MatrixWorkspace_sptr temp = this->getProperty("SecondTransmissionRun");
        secondTransmissionRun = temp;
        if (isPropertyDefault("Params"))
        {
          throw std::invalid_argument(
              "If a SecondTransmissionRun has been given, then stitching Params are also required.");
        }
        else
        {
          std::vector<double> params = getProperty("Params");
          stitchingStartQ = params[0];
          stitchingDeltaQ = params[1];
          stitchingEndQ = params[2];
        }
      }
    }

    /*
    WorkspaceIndexList getSpectrumIds(const WorkspaceIndexList& workspaceIds)
    {
      // Get spectrum ID.
            WorkspaceIndexList detectorSpectrumIdRange;
            for(size_t i = 0; i < detectorIndexRange.size(); ++i)
            {
              auto spectrum = inLam->getSpectrum(detectorIndexRange[i]);
              specid_t specId = spectrum->getSpectrumNo();
              detectorSpectrumIdRange.push_back(specId);
            }
    }
    */

    MatrixWorkspace_sptr ReflectometryReductionOne::toLam(MatrixWorkspace_sptr toConvert,
        const WorkspaceIndexList& detectorIndexRange, const int monitorIndex,
        const MinMax& wavelengthMinMax, const MinMax& backgroundMinMax)
    {
      MatrixWorkspace_sptr detectorWS;
      for (size_t i = 0; i < detectorIndexRange.size(); i += 2)
      {
        auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
        cropWorkspaceAlg->initialize();
        cropWorkspaceAlg->setProperty("InputWorkspace", toConvert);
        cropWorkspaceAlg->setProperty("StartWorkspaceIndex", detectorIndexRange[i]);
        cropWorkspaceAlg->setProperty("EndWorkspaceIndex", detectorIndexRange[i + 1]);

        cropWorkspaceAlg->execute();
        MatrixWorkspace_sptr subRange = cropWorkspaceAlg->getProperty("OutputWorkspace");
        if (i == 0)
        {
          detectorWS = subRange;
        }
        else
        {
          auto conjoinWorkspaceAlg = this->createChildAlgorithm("ConjoinWorkspaces");
          conjoinWorkspaceAlg->initialize();
          conjoinWorkspaceAlg->setProperty("InputWorkspace1", detectorWS);
          conjoinWorkspaceAlg->setProperty("InputWorkspace2", subRange);
          conjoinWorkspaceAlg->execute();
          detectorWS = conjoinWorkspaceAlg->getProperty("InputWorkspace1");
        }
      }

      auto convertUnitsAlg = this->createChildAlgorithm("ConvertUnits");
      convertUnitsAlg->initialize();
      convertUnitsAlg->setProperty("InputWorkspace", detectorWS);
      convertUnitsAlg->setProperty("Target", "Wavelength");
      convertUnitsAlg->setProperty("AlignBins", true);
      convertUnitsAlg->execute();
      detectorWS = convertUnitsAlg->getProperty("OutputWorkspace");

      auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
      cropWorkspaceAlg->initialize();
      cropWorkspaceAlg->setProperty("InputWorkspace", detectorWS);
      cropWorkspaceAlg->setProperty("XMin", wavelengthMinMax.get<0>());
      cropWorkspaceAlg->setProperty("XMax", wavelengthMinMax.get<1>());
      cropWorkspaceAlg->execute();
      detectorWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

      return detectorWS;
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void ReflectometryReductionOne::exec()
    {
      MatrixWorkspace_sptr runWS = getProperty("InputWorkspace");

      OptionalMatrixWorkspace_sptr firstTransmissionRun;
      OptionalMatrixWorkspace_sptr secondTransmissionRun;
      OptionalDouble stitchingStartQ;
      OptionalDouble stitchingDeltaQ;
      OptionalDouble stitchingEndQ;

      getTransmissionRunInfo(firstTransmissionRun, secondTransmissionRun, stitchingStartQ, stitchingDeltaQ, stitchingEndQ);

      OptionalDouble theta;
      if (!isPropertyDefault("Theta"))
      {
        double temp = this->getProperty("Theta");
        theta = temp;
      }

      const std::string strAnalysisMode = getProperty("AnalysisMode");
      const bool isPointDetector = (pointDetectorAnalysis.compare(strAnalysisMode) == 0);

      const MinMax wavelengthInterval = this->getMinMax("WavelengthMin","WavelengthMax");
      const MinMax monitorBackgroundWavelengthInterval = getMinMax("MonitorBackgroundWavelengthMin", "MonitorBackgroundWavelengthMax");
      const MinMax monitorIntegrationWavelengthInterval = getMinMax("MonitorIntegrationWavelengthMin", "MonitorIntegrationWavelengthMax");

      const WorkspaceIndexList indexList = getWorkspaceIndexList();

      OptionalWorkspaceIndexes regionOfInterest;
      fetchOptionalLowerUpperPropertyValue("RegionOfInterest", isPointDetector, regionOfInterest);

      OptionalWorkspaceIndexes directBeam;
      fetchOptionalLowerUpperPropertyValue("RegionOfDirectBeam", isPointDetector, directBeam);

      const int i0MonitorIndex = getProperty("I0MonitorIndex");

      auto outWS = toLam(runWS, indexList, i0MonitorIndex, wavelengthInterval, monitorBackgroundWavelengthInterval);

      setProperty("OutputWorkspace", outWS);

    }

  } // namespace Algorithms
} // namespace Mantid
