#ifndef MANTID_API_SCRIPTBUILDER_H_
#define MANTID_API_SCRIPTBUILDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/HistoryView.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/PropertyHistory.h"

#include <sstream>

namespace Mantid {
namespace API {

/** @class ScriptBuilder

    This class build a sttring which cana be executed as a python script.

    @author Samuel Jackson, ISIS, RAL
    @date 21/01/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

class MANTID_API_DLL ScriptBuilder {
public:
  ScriptBuilder(boost::shared_ptr<HistoryView> view,
                std::string versionSpecificity = "old",
                bool appendTimestamp = false,
                bool ignoreGroupWorkspaces = false);
  virtual ~ScriptBuilder() = default;
  /// build a python script from the history view
  const std::string build();

private:
  void writeHistoryToStream(std::ostringstream &os,
                            std::vector<HistoryItem>::const_iterator &iter,
                            int depth = 1);
  void buildChildren(std::ostringstream &os,
                     std::vector<HistoryItem>::const_iterator &iter,
                     int depth = 1);
  const std::string buildCommentString(const AlgorithmHistory &algHistory);
  const std::string buildAlgorithmString(const AlgorithmHistory &algHistory);
  const std::string
  buildPropertyString(const Mantid::Kernel::PropertyHistory &propHistory);
  void createStringForAlg(
      std::ostringstream &os,
      boost::shared_ptr<const Mantid::API::AlgorithmHistory> &algHistory);

  const std::vector<HistoryItem> m_historyItems;
  std::string m_output;
  std::string m_versionSpecificity;
  bool m_timestampCommands;
  bool m_projectRecovery;
  std::vector<std::string> m_algsToIgnore = {
      "GroupWorkspaces",
      "EnggSaveGSASIIFitResultsToHDF5",
      "EnggSaveSinglePeakFitResultsToHDF5",
      "ExampleSaveAscii",
      "SANSSave",
      "SaveANSTOAscii",
      "SaveAscii",
      "SaveBankScatteringAngles",
      "SaveCSV",
      "SaveCalFile",
      "SaveCanSAS1D",
      "SaveDaveGrp",
      "SaveDetectorsGrouping",
      "SaveDiffCal",
      "SaveDiffFittingAscii",
      "SaveDspacemap",
      "SaveFITS",
      "SaveFocusedXYE",
      "SaveFullprofResolution",
      "SaveGDA",
      "SaveGEMMAUDParamFile",
      "SaveGSASInstrumentFile",
      "SaveGSS",
      "SaveHKL",
      "SaveILLCosmosAscii",
      "SaveISISNexus",
      "SaveIsawDetCal",
      "SaveIsawPeaks",
      "SaveIsawQvector",
      "SaveIsawUB",
      "SaveLauenorm",
      "SaveMD",
      "SaveMDWorkspaceToVTK",
      "SaveMask",
      "SaveNISTDAT",
      "SaveNXSPE",
      "SaveNXTomo",
      "SaveNXcanSAS",
      "SaveNexus",
      "SaveNexusPD",
      "SaveNexusProcessed",
      "SaveOpenGenieAscii",
      "SavePAR",
      "SavePDFGui",
      "SavePHX",
      "SaveParameterFile",
      "SavePlot1D",
      "SavePlot1DAsJson",
      "SaveRKH",
      "SaveReflCustomAscii",
      "SaveReflThreeColumnAscii",
      "SaveReflections",
      "SaveReflectometryAscii",
      "SaveSESANS",
      "SaveSPE",
      "SaveTBL",
      "SaveToSNSHistogramNexus",
      "SaveVTK",
      "SaveVulcanGSS",
      "SaveYDA",
      "SaveZODS"};
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_SCRIPTBUILDER_H_*/
