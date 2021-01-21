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
/** @class DataPostprocessorAlgorithm

DataPostprocessorAlgorithm defines a post-processor algorithm responsible for
post-processing rows belonging to the same group in a Data Processor UI.
*/
class EXPORT_OPT_MANTIDQT_COMMON PostprocessingAlgorithm : public ProcessingAlgorithmBase {
public:
  // Constructor
  PostprocessingAlgorithm(const QString &name, const QString &prefix = "",
                          const std::set<QString> &blacklist = std::set<QString>());
  // Delegating constructor
  PostprocessingAlgorithm(const QString &name, const QString &prefix, const QString &blacklist);
  // Default constructor
  PostprocessingAlgorithm();
  // Destructor
  virtual ~PostprocessingAlgorithm();
  // The name of the input workspace property
  QString inputProperty() const;
  // The name of the output workspace property
  QString outputProperty() const;
  // The number of output workspace properties (currently only 1)
  size_t numberOfOutputProperties() const;
  // The prefix of the output property
  QString prefix() const;

private:
  // The prefix of the output workspace
  QString m_prefix;
  // The name of the input property
  QString m_inputProp;
  // The name of the output property
  QString m_outputProp;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt