#ifndef MANTID_API_EXPERIMENTINFO_H_
#define MANTID_API_EXPERIMENTINFO_H_
    
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"

#include "MantidGeometry/Instrument.h"

#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/DeltaEMode.h"

#include <list>

namespace Mantid
{
  //---------------------------------------------------------------------------
  // Forward declaration
  //---------------------------------------------------------------------------
  namespace Geometry
  {
    class ParameterMap;
  }

namespace API
{
  //---------------------------------------------------------------------------
  // Forward declaration
  //---------------------------------------------------------------------------
  class ChopperModel;
  class ModeratorModel;

  /** This class is shared by a few Workspace types
   * and holds information related to a particular experiment/run:
   *
   * - Instrument (with parameter map)
   * - Run object (sample logs)
   * - Sample object (sample info)
   * 
   * @author Janik Zikovsky
   * @date 2011-06-20
   */
  class MANTID_API_DLL ExperimentInfo
  {
  public:
    /// Default constructor
    ExperimentInfo();
    /// Virtual destructor
    virtual ~ExperimentInfo();
    
    /// Copy everything from the given experiment object
    void copyExperimentInfoFrom(const ExperimentInfo * other);
    /// Clone us
    ExperimentInfo * cloneExperimentInfo()const;

    /// Instrument accessors
    void setInstrument(const Geometry::Instrument_const_sptr&);
    /// Returns the parameterized instrument
    Geometry::Instrument_const_sptr getInstrument() const;

    /// Returns the set of parameters modifying the base instrument (const-version)
    const Geometry::ParameterMap& instrumentParameters() const;
    /// Returns a modifiable set of instrument parameters
    Geometry::ParameterMap& instrumentParameters();
    /// Const version
    const Geometry::ParameterMap& constInstrumentParameters() const;
    // Add parameters to the instrument parameter map
    virtual void populateInstrumentParameters();

    /// Set an object describing the source properties and take ownership
    void setModeratorModel(ModeratorModel *source);
    /// Returns a reference to the source properties object
    ModeratorModel & moderatorModel() const;

    /// Set a chopper description specified by index where 0 is closest to the source
    void setChopperModel(ChopperModel *chopper, const size_t index = 0);
    /// Returns a reference to a chopper description
    ChopperModel & chopperModel(const size_t index = 0) const;

    /// Sample accessors
    const Sample& sample() const;
    /// Writable version of the sample object
    Sample& mutableSample();

    /// Run details object access
    const Run & run() const;
    /// Writable version of the run object
    Run& mutableRun();
    /// Access a log for this experiment.
    Kernel::Property * getLog(const std::string & log) const;
    /// Access a single value from a log for this experiment.
    double getLogAsSingleValue(const std::string & log) const;

    /// Utility method to get the run number
    int getRunNumber() const;
    /// Returns the emode for this run
    Kernel::DeltaEMode::Type getEMode() const;
    /// Easy access to the efixed value for this run & detector ID
    double getEFixed(const detid_t detID) const;
    /// Easy access to the efixed value for this run & optional detector
    double getEFixed(const Geometry::IDetector_const_sptr detector = Geometry::IDetector_const_sptr()) const;

    /// Saves this experiment description to the open NeXus file
    void saveExperimentInfoNexus(::NeXus::File * file) const;
    /// Loads an experiment description from the open NeXus file
    void loadExperimentInfoNexus(::NeXus::File * file, std::string & parameterStr);
    /// Populate the parameter map given a string
    void readParameterMap(const std::string & parameterStr);

    /// Returns the start date for this experiment
    std::string getWorkspaceStartDate();

    /// Utility to retrieve the validity dates for the given IDF
    static void getValidFromTo(const std::string& IDFfilename, std::string& outValidFrom, std::string& outValidTo);
    /// Get the IDF using the instrument name and date
    static std::string getInstrumentFilename(const std::string& instrumentName, const std::string& date);
    /// Get the IDF using the instrument name
    static std::string getInstrumentFilename(const std::string& instrumentName);

  protected:

    /// Static reference to the logger class
    static Kernel::Logger& g_log;

    /// Description of the source object
    boost::shared_ptr<ModeratorModel> m_moderatorModel;
    /// Description of the choppers for this experiment.
    std::list<boost::shared_ptr<ChopperModel>> m_choppers;
    /// The information on the sample environment
    Kernel::cow_ptr<Sample> m_sample;
    /// The run information
    Kernel::cow_ptr<Run> m_run;
    /// Parameters modifying the base instrument
    boost::shared_ptr<Geometry::ParameterMap> m_parmap;
    /// The base (unparametrized) instrument
    Geometry::Instrument_const_sptr sptr_instrument;

  private:
    /// Save information about a set of detectors to Nexus
    void saveDetectorSetInfoToNexus (::NeXus::File * file, std::vector<detid_t> detIDs ) const;
  };

  /// Shared pointer to ExperimentInfo
  typedef boost::shared_ptr<ExperimentInfo> ExperimentInfo_sptr;

  /// Shared pointer to const ExperimentInfo
  typedef boost::shared_ptr<const ExperimentInfo> ExperimentInfo_const_sptr;


} // namespace Mantid
} // namespace API

#endif  /* MANTID_API_EXPERIMENTINFO_H_ */
