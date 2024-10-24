// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <stdexcept>
#include <string>

#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
  This class is used in Catalog Search service to set/get all the inputs to
  search for.
*/
class MANTID_ICAT_DLL CatalogSearchParam {
public:
  /// constructor
  CatalogSearchParam();

  /// Set the start run to search for.
  void setRunStart(const double &startRun);
  /// Set the end run to search for.
  void setRunEnd(const double &endRun);
  /// Set the instrument to search for.
  void setInstrument(const std::string &instrName);
  /// Set the start date to search for.
  void setStartDate(const time_t &startDate);
  /// Set the end date to search for.
  void setEndDate(const time_t &endDate);
  /// Set the keywords to search for.
  void setKeywords(const std::string &keywords);
  /// Set the investigation name to search for.
  void setInvestigationName(const std::string &instName);
  /// Set the datafile name to search for.
  void setDatafileName(const std::string &datafileName);
  /// Set the sample name to search for.
  void setSampleName(const std::string &sampleName);
  /// Set the investigators name to search for.
  void setInvestigatorSurName(const std::string &investigatorName);
  /// Set the investigation type to search for.
  void setInvestigationType(const std::string &invstType);
  /// Set the "my data only" flag to search only user's data if true.
  void setMyData(bool flag);
  /// Set the investigation id to search for.
  void setInvestigationId(const std::string &);

  /// Get the start run from user input.
  const double &getRunStart() const;
  /// Get the end run.
  const double &getRunEnd() const;
  /// Get the instrument name.
  const std::string &getInstrument() const;
  /// Get the investigation start date.
  const time_t &getStartDate() const;
  /// Get the investigation end date.
  const time_t &getEndDate() const;
  /// Get the keywords to search investigations for.
  const std::string &getKeywords() const;
  /// Get the name of the investigation to search for.
  const std::string &getInvestigationName() const;
  /// Get the datafile name.
  const std::string &getDatafileName() const;
  /// Get the sample name.
  const std::string &getSampleName() const;
  /// Get the investigators name.
  const std::string &getInvestigatorSurName() const;
  /// Get the investigation type.
  const std::string &getInvestigationType() const;
  /// Get the "my data only" flag.
  bool getMyData() const;
  /// Get the investigation id.
  const std::string &getInvestigationId() const;
  /// Saves the start/end date times to time_t value.
  time_t getTimevalue(const std::string &inputDate);

private:
  /// start run number
  double m_startRun;
  /// end run number
  double m_endRun;
  /// instrument name
  std::string m_instrName;
  /// search keywords
  std::string m_keywords;
  /// start date
  time_t m_startDate;
  /// end date
  time_t m_endDate;
  /// investigation anme
  std::string m_investigationName;
  /// Datafile name
  std::string m_datafileName;
  /// sample name
  std::string m_sampleName;
  /// investigator surname
  std::string m_investigatorSurname;
  /// investigation type
  std::string m_investigationType;
  /// My data checkbox
  bool m_myData;
  /// investigation id
  std::string m_investigationId;
};
} // namespace ICat
} // namespace Mantid
