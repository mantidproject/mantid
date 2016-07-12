#include "MantidQtCustomInterfaces/Tomography/TomoSystemSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomoSystemSettings::g_defSamplesDirPrefix = "data";
const std::string TomoSystemSettings::g_defFlatsDirPrefix = "flat";
const std::string TomoSystemSettings::g_defDarksDirPrefix = "dark";

const std::string TomoSystemSettings::g_defOutputPathCompPreProcessed =
    "pre_processed";

const std::string TomoSystemSettings::g_defOutputPathCompReconst = "processed";

/**
 * Default initialization which should be enough in most cases.
 */
TomoSystemSettings::TomoSystemSettings()
    : m_local(), m_remote(),
      m_pathComponents({"data", "RB number", "experiment name", "input type"}),
      m_samplesDirPrefix(g_defSamplesDirPrefix),
      m_flatsDirPrefix(g_defFlatsDirPrefix),
      m_darksDirPrefix(g_defDarksDirPrefix),
      m_outputPathCompPreProcessed(g_defOutputPathCompPreProcessed),
      m_outputPathCompReconst(g_defOutputPathCompReconst) {}

} // namespace CustomInterfaces
} // namespace MantidQt
