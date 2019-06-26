// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
    */

class MANTID_API_DLL ScriptBuilder {
public:
  ScriptBuilder(
      boost::shared_ptr<HistoryView> view,
      std::string versionSpecificity = "old", bool appendTimestamp = false,
      std::vector<std::string> ignoreTheseAlgs = {},
      std::vector<std::vector<std::string>> ignoreTheseAlgProperties = {},
      bool appendExecCount = false);
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
  buildPropertyString(const Mantid::Kernel::PropertyHistory &propHistory,
                      const std::string &algName);
  void createStringForAlg(
      std::ostringstream &os,
      boost::shared_ptr<const Mantid::API::AlgorithmHistory> &algHistory);

  const std::vector<HistoryItem> m_historyItems;
  std::string m_output;
  std::string m_versionSpecificity;
  bool m_timestampCommands;
  std::vector<std::string> m_algsToIgnore;
  std::vector<std::vector<std::string>> m_propertiesToIgnore;
  bool m_execCount;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_SCRIPTBUILDER_H_*/
