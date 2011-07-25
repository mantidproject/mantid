#include "MantidAlgorithms/RemovePromptPulse.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"

using std::string;
namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RemovePromptPulse)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RemovePromptPulse::RemovePromptPulse()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RemovePromptPulse::~RemovePromptPulse()
  {
  }
  
  const string RemovePromptPulse::name() const
  {
    return "RemovePromptPulse";
  }

  int RemovePromptPulse::version() const
  {
    return 1;
  }

  const string RemovePromptPulse::category() const {
    return "General";
  }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void RemovePromptPulse::initDocs()
  {
    string msg("Remove the prompt pulse for a time of flight measurement.");
    this->setWikiSummary(msg);
    this->setOptionalMessage(msg);
    //this->setWikiDescription("TODO: Enter the text to be placed in the Description section of the wiki page.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void RemovePromptPulse::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, new WorkspaceUnitValidator<>("TOF")), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");

    BoundedValidator<double> *validator = new BoundedValidator<double>;
    validator->setLower(0.0);
    declareProperty("Width", Mantid::EMPTY_DBL(), validator,
                    "The width of the time of flight (in microseconds) to remove from the data." );
    validator = new BoundedValidator<double>;
    validator->setLower(0.0);
    declareProperty("Frequency", Mantid::EMPTY_DBL(), validator,
                    "The frequency of the source (in Hz) used to calculate the minimum time of flight to filter." );
  }

  //----------------------------------------------------------------------------------------------
namespace { // anonymous namespace begin
  double getMedian(const API::Run& run, const std::string& name)
  {

    Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( run.getLogData(name) );
    if (!log)
      return Mantid::EMPTY_DBL();

    Kernel::TimeSeriesPropertyStatistics stats = log->getStatistics();
    return stats.median;
  }

  void getTofRange(MatrixWorkspace_const_sptr wksp, double& tmin, double& tmax)
  {
    DataObjects::EventWorkspace_const_sptr eventWksp = boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(wksp);

    bool isEvent = false;
    if (isEvent)
    {
      tmin = eventWksp->getTofMin();
      tmax = eventWksp->getTofMax();
      return;
    }

    int numberOfSpectra = static_cast<int>(wksp->getNumberHistograms());
    tmin = std::numeric_limits<double>::max();
    tmax = -1. * tmin;
    double temp;
    for (int workspaceIndex = 0; workspaceIndex < numberOfSpectra; workspaceIndex++)
    {
      // get the minimum value
      temp = wksp->dataX(workspaceIndex).front();
      if (temp < tmin)
        tmin = temp;

      // get the maximum value
      temp = wksp->dataX(workspaceIndex).back();
      if (temp > tmax)
        tmax = temp;
    }
  }
 } // anonymous namespace end

  /** Execute the algorithm.
   */
  void RemovePromptPulse::exec()
  {
    // verify there is a width parameter specified
    double width = this->getProperty("Width");
    if (this->isEmpty(width))
    {
      throw std::runtime_error("Failed to specify \'Width\' parameter");
    }

    // need the input workspace in general
    API::MatrixWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");

    // get the frequency
    double frequency = this->getProperty("Frequency");
    if(this->isEmpty(frequency)) // it wasn't specified so try divination
    {
      frequency = this->getFrequency(inputWS->run());
      if (this->isEmpty(frequency)) {
        throw std::runtime_error("Failed to determine the frequency");
      }
    }
    g_log.information() << "Using frequency of " << frequency << "Hz\n";
    double period = 1000000./frequency; // period in microseconds

    // determine the overall tof window for the data
    double tmin;
    double tmax;
    getTofRange(inputWS, tmin, tmax);
    g_log.information() << "Data tmin=" << tmin << ", tmax=" << tmax << ", period=" << period << " microseconds\n";

    // get the first prompt pulse
    double left = 0.;
    while (left < tmin)
    {
      left += period;
    }
    if (left > tmax)
    {
      g_log.notice() << "Not applying filter since prompt pulse is not in data range (" << left << " > " << tmax
                      << " microseconds, period = " << period << ")\n";
      setProperty("OutputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(inputWS));
      return;
    }
    double right = left + width;
    g_log.notice() << "Filtering tmin=" << left << ", tmax=" << right << " microseconds\n";

    // run maskbins to do the work on the first prompt pulse
    IAlgorithm_sptr algo = this->createSubAlgorithm("MaskBins");
    algo->setProperty<MatrixWorkspace_sptr>("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(inputWS));
    algo->setProperty<double>("XMin", left);
    algo->setProperty<double>("XMax", right);
    algo->executeAsSubAlg();

    // copy over the output workspace
    MatrixWorkspace_sptr outputWS = algo->getProperty("OutputWorkspace");
    setProperty("OutputWorkspace", outputWS);

    // verify that there isn't another window to deal with
    if (left + period < tmax)
    {
      g_log.warning() << "There is more than one prompt pulse possible in the data, only the first was filtered "
                      << "(at TOF = " << left << ", not " << (left+period) << ")\n";
    }
  }

  double RemovePromptPulse::getFrequency(const API::Run& run)
  {
    double candidate;

    candidate = getMedian(run, "Frequency");
    if (!(this->isEmpty(candidate)))
      return candidate;

    candidate = getMedian(run, "frequency");
    if (!(this->isEmpty(candidate)))
      return candidate;

    candidate = getMedian(run, "FREQUENCY");
    if (!(this->isEmpty(candidate)))
      return candidate;

    // give up
    return Mantid::EMPTY_DBL();
  }
} // namespace Mantid
} // namespace Algorithms

