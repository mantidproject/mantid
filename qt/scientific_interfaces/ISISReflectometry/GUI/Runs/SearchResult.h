// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class SearchResult

This class hold information about a run returned from the search results. It
takes the run number and title and parses the group name and angle out of the
title.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL SearchResult {
public:
  SearchResult(const std::string &runNumber, const std::string &title);
  SearchResult(const std::string &runNumber, const std::string &title, const std::string &groupName,
               const std::string &theta, const std::string &error, const std::string &excludeReason,
               const std::string &comment);

  const std::string &runNumber() const;
  const std::string &title() const;
  const std::string &groupName() const;
  const std::string &theta() const;
  bool hasError() const;
  const std::string &error() const;
  bool exclude() const;
  const std::string &excludeReason() const;
  void addExcludeReason(std::string const &error);
  bool hasComment() const;
  const std::string &comment() const;
  void addComment(std::string const &error);

private:
  std::string m_runNumber;
  std::string m_title;
  std::string m_groupName;
  std::string m_theta;
  std::string m_error;
  std::string m_excludeReason;
  std::string m_comment;

  void parseRun(std::string const &runNumber);
  void parseMetadataFromTitle();
  void addError(std::string const &error);
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(SearchResult const &lhs, SearchResult const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(SearchResult const &lhs, SearchResult const &rhs);

using SearchResults = std::vector<SearchResult>;
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
