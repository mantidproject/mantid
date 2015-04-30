#ifndef MANTID_API_EXPERIMENTINFO_H_
#define MANTID_API_EXPERIMENTINFO_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"

#include "MantidGeometry/Instrument.h"
#include "MantidAPI/SpectraDetectorTypes.h"

#include "MantidKernel/DeltaEMode.h"

#include <list>

namespace Mantid {
//---------------------------------------------------------------------------
// Forward declaration
//---------------------------------------------------------------------------
namespace Geometry {
class ParameterMap;
}

namespace API {
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
 */
class MANTID_API_DLL ExperimentInfo {
public:
  /// Default constructor
  ExperimentInfo();
  /// Virtual destructor
  virtual ~ExperimentInfo();
  /// Copy constructor
  ExperimentInfo(const ExperimentInfo &);
  /// Copy everything from the given experiment object
  void copyExperimentInfoFrom(const ExperimentInfo *other);
  /// Clone us
  virtual ExperimentInfo *cloneExperimentInfo() const;

  /// Returns a string description of the object
  virtual const std::string toString() const;

  /// Instrument accessors
  void setInstrument(const Geometry::Instrument_const_sptr &instr);
  /// Returns the parameterized instrument
  virtual Geometry::Instrument_const_sptr getInstrument() const;

  /// Returns the set of parameters modifying the base instrument
  /// (const-version)
  virtual const Geometry::ParameterMap &instrumentParameters() const;
  /// Returns a modifiable set of instrument parameters
  virtual Geometry::ParameterMap &instrumentParameters();
  /// Const version
  virtual const Geometry::ParameterMap &constInstrumentParameters() const;
  // Add parameters to the instrument parameter map
  virtual void populateInstrumentParameters();

  /// Replaces current parameter map with copy of given map
  virtual void replaceInstrumentParameters(const Geometry::ParameterMap &pmap);
  /// exchange contents of current parameter map with contents of other map)
  virtual void swapInstrumentParameters(Geometry::ParameterMap &pmap);

  /// Cache a lookup of grouped detIDs to member IDs
  virtual void cacheDetectorGroupings(const det2group_map &mapping);
  /// Returns the detector IDs that make up the group that this ID is part of
  virtual const std::vector<detid_t> &getGroupMembers(const detid_t detID) const;
  /// Get a detector or detector group from an ID
  virtual  Geometry::IDetector_const_sptr getDetectorByID(const detid_t detID) const;

  /// Set an object describing the source properties and take ownership
  virtual void setModeratorModel(ModeratorModel *source);
  /// Returns a reference to the source properties object
  virtual ModeratorModel &moderatorModel() const;

  /// Set a chopper description specified by index where 0 is closest to the
  /// source
  virtual void setChopperModel(ChopperModel *chopper, const size_t index = 0);
  /// Returns a reference to a chopper description
  virtual ChopperModel &chopperModel(const size_t index = 0) const;

  /// Sample accessors
  virtual const Sample &sample() const;
  /// Writable version of the sample object
  virtual Sample &mutableSample();

  /// Run details object access
  virtual const Run &run() const;
  /// Writable version of the run object
  virtual Run &mutableRun();
  /// Access a log for this experiment.
  virtual Kernel::Property *getLog(const std::string &log) const;
  /// Access a single value from a log for this experiment.
  virtual double getLogAsSingleValue(const std::string &log) const;

  /// Utility method to get the run number
  virtual int getRunNumber() const;
  /// Returns the emode for this run
  virtual Kernel::DeltaEMode::Type getEMode() const;
  /// Easy access to the efixed value for this run & detector ID
  virtual double getEFixed(const detid_t detID) const;
  /// Easy access to the efixed value for this run & optional detector
  virtual double getEFixed(const Geometry::IDetector_const_sptr detector =
                       Geometry::IDetector_const_sptr()) const;
  /// Set the efixed value for a given detector ID
  virtual void setEFixed(const detid_t detID, const double value);

  /// Saves this experiment description to the open NeXus file
  virtual void saveExperimentInfoNexus(::NeXus::File *file) const;
  /// Loads an experiment description from the open NeXus file
  virtual void loadExperimentInfoNexus(::NeXus::File *file, std::string &parameterStr);
  /// Load the instrument from an open NeXus file.
  virtual void loadInstrumentInfoNexus(::NeXus::File *file, std::string &parameterStr);
  /// Load the sample and log info from an open NeXus file.
  virtual void loadSampleAndLogInfoNexus(::NeXus::File *file);
  /// Populate the parameter map given a string
  virtual void readParameterMap(const std::string &parameterStr);

  /// Returns the start date for this experiment (or current time if no info
  /// available)
  virtual std::string getWorkspaceStartDate() const;

  // run/experiment stat time if available, empty otherwise
  virtual std::string getAvailableWorkspaceStartDate() const;
  // run end time if available, empty otherwise
  virtual std::string getAvailableWorkspaceEndDate() const;

  /// Utility to retrieve the validity dates for the given IDF
  static void getValidFromTo(const std::string &IDFfilename,
                             std::string &outValidFrom,
                             std::string &outValidTo);
  /// Get the IDF using the instrument name and date
  static std::string getInstrumentFilename(const std::string &instrumentName,
                                           const std::string &date = "");

protected:
  /// Description of the source object
  boost::shared_ptr<ModeratorModel> m_moderatorModel;
  /// Description of the choppers for this experiment.
  std::list<boost::shared_ptr<ChopperModel>> m_choppers;
  /// The information on the sample environment
  boost::shared_ptr<Sample> m_sample;
  /// The run information
  boost::shared_ptr<Run> m_run;
  /// Parameters modifying the base instrument
  boost::shared_ptr<Geometry::ParameterMap> m_parmap;
  /// The base (unparametrized) instrument
  Geometry::Instrument_const_sptr sptr_instrument;

private:
  /// Fill with given instrument parameter
  void populateWithParameter(Geometry::ParameterMap &paramMap,
                             const std::string &name,
                             const Geometry::XMLInstrumentParameter &paramInfo,
                             const Run &runData);

  /// Detector grouping information
  det2group_map m_detgroups;
  /// Mutex to protect against cow_ptr copying
  mutable Poco::Mutex m_mutex;
};

/// Shared pointer to ExperimentInfo
typedef boost::shared_ptr<ExperimentInfo> ExperimentInfo_sptr;

/// Shared pointer to const ExperimentInfo
typedef boost::shared_ptr<const ExperimentInfo> ExperimentInfo_const_sptr;

} // namespace Mantid
} // namespace API

#endif /* MANTID_API_EXPERIMENTINFO_H_ */
