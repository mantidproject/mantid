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
/** @class PreprocessingAlgorithm

PreprocessingAlgorithm defines a pre-processor algorithm that will
be
responsible for pre-processsing a specific column in a Data Processor UI.
*/
class EXPORT_OPT_MANTIDQT_COMMON PreprocessingAlgorithm : public ProcessingAlgorithmBase {
public:
  // Constructor
  PreprocessingAlgorithm(const QString &name, const QString &prefix = "", const QString &separator = "",
                         const std::set<QString> &blacklist = std::set<QString>());
  // Delegating constructor
  PreprocessingAlgorithm(const QString &name, const QString &prefix, const QString &separator,
                         const QString &blacklist);
  // Default constructor
  PreprocessingAlgorithm();
  // Destructor
  virtual ~PreprocessingAlgorithm();

  // The name of the lhs input property
  QString lhsProperty() const;
  // The name of the rhs input property
  QString rhsProperty() const;
  // The name of the output property
  QString outputProperty() const;
  // The prefix to add to the output property
  QString prefix() const;
  // The separator to use between values in the output property
  QString separator() const;

private:
  // A prefix to the name of the pre-processed output ws
  QString m_prefix;
  // A separator between values in the pre-processed output ws name
  QString m_separator;
  // The name of the LHS input property
  QString m_lhs;
  // The name of the RHS input property
  QString m_rhs;
  // The name of the output proerty
  QString m_outProperty;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt