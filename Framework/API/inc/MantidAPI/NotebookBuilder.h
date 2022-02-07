// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** @class NotebookBuilder

    This class builds an ipython notebook of an algorithm's history, using
    NotebookWriter.
    */

#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/HistoryView.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/PropertyHistory.h"

#include <sstream>

namespace Mantid {
namespace API {

class MANTID_API_DLL NotebookBuilder {
public:
  NotebookBuilder(const std::shared_ptr<HistoryView> &view, std::string versionSpecificity = "old");
  virtual ~NotebookBuilder() = default;
  /// build an ipython notebook from the history view
  const std::string build(const std::string &ws_name, const std::string &ws_title, const std::string &ws_comment);

private:
  void writeHistoryToStream(std::vector<HistoryItem>::const_iterator &iter);
  void buildChildren(std::vector<HistoryItem>::const_iterator &iter);
  const std::string buildAlgorithmString(const AlgorithmHistory_const_sptr &algHistory);
  const std::string buildPropertyString(const Mantid::Kernel::PropertyHistory_const_sptr &propHistory);

  const std::vector<HistoryItem> m_historyItems;
  std::string m_output;
  std::string m_versionSpecificity;
  std::unique_ptr<NotebookWriter> m_nb_writer;
};

} // namespace API
} // namespace Mantid
