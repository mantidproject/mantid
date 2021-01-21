// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithmBase.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class ProcessingAlgorithm

ProcessingAlgorithm defines a processing algorithm that will
perform the
reduction in a Data ProcessorProcessing UI.
*/
class EXPORT_OPT_MANTIDQT_COMMON ProcessingAlgorithm : public ProcessingAlgorithmBase {
public:
  ProcessingAlgorithm();
  // Constructor
  ProcessingAlgorithm(const QString &name, std::vector<QString> prefix, std::size_t postprocessedOutputPrefixIndex,
                      const std::set<QString> &blacklist = std::set<QString>(), const int version = -1);
  // Delegating constructor
  ProcessingAlgorithm(const QString &name, QString const &prefix, std::size_t postprocessedOutputPrefixIndex,
                      QString const &blacklist = "", const int version = -1);
  // Destructor
  virtual ~ProcessingAlgorithm();
  // The number of output properties
  size_t numberOfOutputProperties() const;
  // The prefix for this output property
  QString prefix(size_t index) const;
  // The name of this input property
  QString inputPropertyName(size_t index) const;
  // The name of this output property
  QString outputPropertyName(size_t index) const;
  // The prefix for the default output ws property
  QString defaultOutputPrefix() const;
  // The default output ws property
  QString defaultOutputPropertyName() const;
  // The default input ws property
  QString defaultInputPropertyName() const;
  // The prefix for the postprocessed output ws property
  QString postprocessedOutputPrefix() const;
  // The postprocessed output ws property
  QString postprocessedOutputPropertyName() const;
  // The input properties
  std::vector<QString> inputProperties() const;
  // The output properties
  std::vector<QString> outputProperties() const;
  // The prefixes for the output properties
  std::vector<QString> prefixes() const;

private:
  bool isValidOutputPrefixIndex(std::size_t outputPrefixIndex) const;
  void ensureValidPostprocessedOutput() const;
  std::size_t m_postprocessedOutputPrefixIndex;
  // The prefix of the output workspace(s)
  std::vector<QString> m_prefix;
  // The names of the input workspace properties
  std::vector<QString> m_inputProperties;
  // The names of the output workspace properties
  std::vector<QString> m_outputProperties;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt