#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"

#include <boost/lexical_cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {

ToolConfigTomoPy::ToolConfigTomoPy()
    : TomoRecToolConfig(""), m_pathOut(""), m_pathDark(""), m_pathOpen(""),
      m_pathSample("") {}

ToolConfigTomoPy::ToolConfigTomoPy(const std::string &runnable,
                                   const std::string &pathOut,
                                   const std::string &pathDark,
                                   const std::string &pathOpen,
                                   const std::string &pathSample)
    : TomoRecToolConfig(runnable), m_pathOut(pathOut), m_pathDark(pathDark),
      m_pathOpen(pathOpen), m_pathSample(pathSample) {}

std::string ToolConfigTomoPy::makeCmdLineOptions() const {
  return "--input-path=" + m_pathSample + " --output-path=" + m_pathOut;
}

ToolConfigAstraToolbox::ToolConfigAstraToolbox()
    : TomoRecToolConfig(""), m_pathOut(""), m_pathDark(""), m_pathOpen(""),
      m_pathSample("") {}

ToolConfigAstraToolbox::ToolConfigAstraToolbox(const std::string &runnable,
                                               const std::string &pathOut,
                                               const std::string &pathDark,
                                               const std::string &pathOpen,
                                               const std::string &pathSample)
    : TomoRecToolConfig(runnable), m_pathOut(pathOut), m_pathDark(pathDark),
      m_pathOpen(pathOpen), m_pathSample(pathSample) {}

std::string ToolConfigAstraToolbox::makeCmdLineOptions() const {
  return " --input-path=" + m_pathSample + " --output-path=" + m_pathOut;
}

} // namespace CustomInterfaces
} // namespace MantidQt
