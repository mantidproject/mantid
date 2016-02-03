#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"

#include <boost/lexical_cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {

// pairs of name-in-the-tool, human-readable-name
const std::vector<
    std::pair<std::string, std::string>> ToolConfigTomoPy::g_tomopyMethods = {
    std::make_pair("gridrec", "gridrec: Fourier grid reconstruction "
                              "algorithm (Dowd, 19999; Rivers, 2006)"),
    std::make_pair("sirt",
                   "sirt: Simultaneous algebraic reconstruction technique"),

    std::make_pair("art",
                   "art: Algebraic reconstruction technique (Kak, 1998)"),
    std::make_pair("bart", "bart: Block algebraic reconstruction technique."),
    std::make_pair("fbp", "fbp: Filtered back-projection algorithm"),
    std::make_pair("mlem", "mlem: Maximum-likelihood expectation maximization "
                           "algorithm (Dempster, 1977)"),
    std::make_pair("osem", "osem: Ordered-subset expectation maximization "
                           "algorithm (Hudson, 1994)"),
    std::make_pair("ospml_hybrid",
                   "ospml_hybrid: Ordered-subset penalized maximum "
                   "likelihood algorithm with weighted "
                   "linear and quadratic penalties"),
    std::make_pair("ospml_quad", "ospml_quad: Ordered-subset penalized maximum "
                                 "likelihood algorithm with quadratic "
                                 "penalties"),
    std::make_pair("pml_hybrid",
                   "pml_hybrid: Penalized maximum likelihood algorithm "
                   "with weighted linear and quadratic "
                   "penalties (Chang, 2004)"),
    std::make_pair("pml_quad", "pml_quad: Penalized maximum likelihood "
                               "algorithm with quadratic penalty"),
};

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

const std::vector<std::pair<
    std::string, std::string>> ToolConfigAstraToolbox::g_astraMethods = {
    std::make_pair("FBP3D_CUDA", "FBP 3D: Filtered Back-Propagation"),
    std::make_pair(
        "SIRT3D_CUDA",
        "SIRT 3D: Simultaneous Iterative Reconstruction Technique algorithm"),
    std::make_pair("CGLS3D_CUDA",
                   "CGLS 3D: Conjugate gradient least square algorithm"),
    std::make_pair("FDK_CUDA", "FDK 3D: Feldkamp-Davis-Kress algorithm for "
                               "3D circular cone beam data sets")};

} // namespace CustomInterfaces
} // namespace MantidQt
