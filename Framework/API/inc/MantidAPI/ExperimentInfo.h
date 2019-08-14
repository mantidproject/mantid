// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_EXPERIMENTINFO_H_
#define MANTID_API_EXPERIMENTINFO_H_

#include "MantidAPI/DllConfig.h"

#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidGeometry/Instrument_fwd.h"

#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/cow_ptr.h"

#include <mutex>

namespace Mantid {
class SpectrumDefinition;
namespace Kernel {
class Property;
}
namespace Beamline {
class ComponentInfo;
class DetectorInfo;
class SpectrumInfo;
} // namespace Beamline
namespace Geometry {
class ComponentInfo;
class DetectorInfo;
class IDetector;
class ParameterMap;
class XMLInstrumentParameter;
} // namespace Geometry

namespace API {
class Run;
class Sample;
class SpectrumInfo;

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
  const std::string toString() const;

  /// Instrument accessors
  void setInstrument(const Geometry::Instrument_const_sptr &instr);
  /// Returns the parameterized instrument
  Geometry::Instrument_const_sptr getInstrument() const;

  /// Returns the set of parameters modifying the base instrument
  /// (const-version)
  const Geometry::ParameterMap &instrumentParameters() const;
  /// Returns a modifiable set of instrument parameters
  Geometry::ParameterMap &instrumentParameters();
  /// Const version
  const Geometry::ParameterMap &constInstrumentParameters() const;
  // Add parameters to the instrument parameter map
  void populateInstrumentParameters();

  void setNumberOfDetectorGroups(const size_t count) const;
  void setDetectorGrouping(const size_t index,
                           const std::set<detid_t> &detIDs) const;

  /// Sample accessors
  const Sample &sample() const;
  /// Writable version of the sample object
  Sample &mutableSample();

  /// Run details object access
  const Run &run() const;
  /// Writable version of the run object
  Run &mutableRun();
  void setSharedRun(Kernel::cow_ptr<Run> run);
  Kernel::cow_ptr<Run> sharedRun();

  /// Access a log for this experiment.
  Kernel::Property *getLog(const std::string &log) const;
  /// Access a single value from a log for this experiment.
  double getLogAsSingleValue(const std::string &log) const;

  /// Utility method to get the run number
  int getRunNumber() const;
  /// Returns the emode for this run
  Kernel::DeltaEMode::Type getEMode() const;
  /// Easy access to the efixed value for this run & detector ID
  double getEFixed(const detid_t detID) const;
  /// Easy access to the efixed value for this run & optional detector
  double getEFixed(const boost::shared_ptr<const Geometry::IDetector> detector =
                       boost::shared_ptr<const Geometry::IDetector>{
                           nullptr}) const;
  /// Set the efixed value for a given detector ID
  void setEFixed(const detid_t detID, const double value);

  /// Saves this experiment description to the open NeXus file
  void saveExperimentInfoNexus(::NeXus::File *file) const;
  /// Loads an experiment description from the open NeXus file
  void loadExperimentInfoNexus(const std::string &nxFilename,
                               ::NeXus::File *file, std::string &parameterStr);
  /// Load the instrument from an open NeXus file.
  void loadInstrumentInfoNexus(const std::string &nxFilename,
                               ::NeXus::File *file, std::string &parameterStr);
  /// Load the instrument from an open NeXus file without reading any parameters
  void loadInstrumentInfoNexus(const std::string &nxFilename,
                               ::NeXus::File *file);
  /// Load instrument parameters from an open Nexus file in Instument group if
  /// found there
  void loadInstrumentParametersNexus(::NeXus::File *file,
                                     std::string &parameterStr);

  /// Load the sample and log info from an open NeXus file.
  void loadSampleAndLogInfoNexus(::NeXus::File *file);
  /// Populate the parameter map given a string
  void readParameterMap(const std::string &parameterStr);

  /// Returns the start date for this experiment (or current time if no info
  /// available)
  std::string getWorkspaceStartDate() const;

  // run/experiment stat time if available, empty otherwise
  std::string getAvailableWorkspaceStartDate() const;
  // run end time if available, empty otherwise
  std::string getAvailableWorkspaceEndDate() const;

  /// Utility to retrieve the validity dates for the given IDF
  static void getValidFromTo(const std::string &IDFfilename,
                             std::string &outValidFrom,
                             std::string &outValidTo);
  /// Utility to retrieve a resource file (IDF, Parameters, ..)
  static std::vector<std::string> getResourceFilenames(
      const std::string &prefix, const std::vector<std::string> &fileFormats,
      const std::vector<std::string> &directoryNames, const std::string &date);
  /// Get the IDF using the instrument name and date
  static std::string getInstrumentFilename(const std::string &instrumentName,
                                           const std::string &date = "");
  const Geometry::DetectorInfo &detectorInfo() const;
  Geometry::DetectorInfo &mutableDetectorInfo();

  const SpectrumInfo &spectrumInfo() const;
  SpectrumInfo &mutableSpectrumInfo();

  const Geometry::ComponentInfo &componentInfo() const;
  Geometry::ComponentInfo &mutableComponentInfo();

  void invalidateSpectrumDefinition(const size_t index);
  void updateSpectrumDefinitionIfNecessary(const size_t index) const;

protected:
  size_t numberOfDetectorGroups() const;
  /// Called as the first operation of most public methods.
  virtual void populateIfNotLoaded() const;

  void setSpectrumDefinitions(
      Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinitions);

  virtual void updateCachedDetectorGrouping(const size_t index) const;
  /// Parameters modifying the base instrument
  boost::shared_ptr<Geometry::ParameterMap> m_parmap;
  /// The base (unparametrized) instrument
  Geometry::Instrument_const_sptr sptr_instrument;

private:
  /// Fill with given instrument parameter
  void populateWithParameter(Geometry::ParameterMap &paramMap,
                             Geometry::ParameterMap &paramMapForPosAndRot,
                             const std::string &name,
                             const Geometry::XMLInstrumentParameter &paramInfo,
                             const Run &runData);

  /// Attempt to load instrument embedded in Nexus file. *file must have
  /// instrument group open.
  void loadEmbeddedInstrumentInfoNexus(::NeXus::File *file,
                                       std::string &instrumentName,
                                       std::string &instrumentXml);

  /// Set the instrument given the name and XML leading from IDF file if XML
  /// string is empty
  void setInstumentFromXML(const std::string &nxFilename,
                           std::string &instrumentName,
                           std::string &instrumentXml);

  // Loads the xml from an instrument file with some basic error handling
  std::string loadInstrumentXML(const std::string &filename);

  /// The information on the sample environment
  Kernel::cow_ptr<Sample> m_sample;
  /// The run information
  Kernel::cow_ptr<Run> m_run;

  /// Detector grouping information
  mutable std::unordered_map<detid_t, size_t> m_det2group;
  void cacheDefaultDetectorGrouping() const; // Not thread-safe
  void invalidateAllSpectrumDefinitions();
  mutable std::once_flag m_defaultDetectorGroupingCached;

  mutable std::unique_ptr<Beamline::SpectrumInfo> m_spectrumInfo;
  mutable std::unique_ptr<SpectrumInfo> m_spectrumInfoWrapper;
  mutable std::mutex m_spectrumInfoMutex;
  // This vector stores boolean flags but uses char to do so since
  // std::vector<bool> is not thread-safe.
  mutable std::vector<char> m_spectrumDefinitionNeedsUpdate;
};

/// Shared pointer to ExperimentInfo
using ExperimentInfo_sptr = boost::shared_ptr<ExperimentInfo>;

/// Shared pointer to const ExperimentInfo
using ExperimentInfo_const_sptr = boost::shared_ptr<const ExperimentInfo>;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_EXPERIMENTINFO_H_ */
