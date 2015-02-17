#include "MantidAPI/FileBackedExperimentInfo.h"

namespace Mantid
{
namespace API
{
namespace {
/// static logger object
Kernel::Logger g_log("FileBackedExperimentInfo");
}

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FileBackedExperimentInfo::FileBackedExperimentInfo(::NeXus::File *file, const std::string groupName)
   : ExperimentInfo(), m_file(file), m_groupName(groupName) {
      m_experimentInfoIsLoaded = false;
  }

  //----------------------------------------------------------------------------------------------
  /// @returns A human-readable description of the object
  const std::string FileBackedExperimentInfo::toString() {
      if (!m_experimentInfoIsLoaded) {
          intialise();
      }

      return ExperimentInfo::toString();
  }

  //------------------------------------------------------------------------------------------------------
  // Private members
  //------------------------------------------------------------------------------------------------------
  /**
   * Here we actually load the data
   */
  void FileBackedExperimentInfo::intialise() {
      std::string parameterStr;
      m_file->openGroup(m_groupName, "NXgroup");
      try {
        // Get the sample, logs, instrument
        this->loadExperimentInfoNexus(m_file, parameterStr);
        // Now do the parameter map
        this->readParameterMap(parameterStr);
      } catch (std::exception &e) {
        g_log.information("Error loading section '" + m_groupName +
                          "' of nxs file.");
        g_log.information(e.what());
      }

      m_experimentInfoIsLoaded = true;
  }

} // namespace API
} // namespace Mantid
