// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QString>

#include <set>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class ProcessingAlgorithmBase

ProcessingAlgorithmBase defines shared code to be used by derived
classes
(PreprocessingAlgorithm, ProcessingAlgorithm and
PostprocessingAlgorithm).
*/
class EXPORT_OPT_MANTIDQT_COMMON ProcessingAlgorithmBase {
public:
  // Default constructor
  ProcessingAlgorithmBase();

  // Constructor
  ProcessingAlgorithmBase(const QString &name, const std::set<QString> &blacklist = std::set<QString>(),
                          const int version = -1);

  // Destructor
  ~ProcessingAlgorithmBase();

  // Returns the input workspaces properties defined for this algorithm
  virtual std::vector<QString> getInputWsProperties() final;

  // Returns the input str list properties defined for this algorithm
  virtual std::vector<QString> getInputStrListProperties() final;

  // Returns the output workspaces properties defined for this algorithm
  virtual std::vector<QString> getOutputWsProperties() final;

  // Returns the name of this algorithm
  virtual QString name() const final { return m_algName; };

  // Returns the blacklist
  virtual std::set<QString> blacklist() const final { return m_blacklist; };

private:
  // Counts number of workspace properties
  void countWsProperties();

  // The name of this algorithm
  QString m_algName;
  // The version of this algorithm
  int m_version;
  // The blacklist
  std::set<QString> m_blacklist;
  // Input ws properties
  std::vector<QString> m_inputWsProperties;
  // Input str list properties
  std::vector<QString> m_inputStrListProperties;
  // Output ws properties
  std::vector<QString> m_OutputWsProperties;

protected:
  // Converts a string to a vector of strings
  static std::vector<QString> convertStringToVector(const QString &text);
  // Converts a string to a set of strings
  static std::set<QString> convertStringToSet(const QString &text);
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt