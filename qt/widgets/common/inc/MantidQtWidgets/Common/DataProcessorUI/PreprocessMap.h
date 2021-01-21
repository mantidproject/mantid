// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <map>

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class PreprocessMap

PreprocessMap defines a pre-processor algorithm that will
be
responsible for pre-processsing a specific column in a Data Processor UI.
*/
class EXPORT_OPT_MANTIDQT_COMMON PreprocessMap {
public:
  // Default constructor
  PreprocessMap();
  // Destructor
  virtual ~PreprocessMap();
  // Add a column to pre-process
  void addElement(const QString &column, const QString &algorithm, const QString &prefix = "",
                  const QString &separator = "", const QString &blacklist = "");
  // Returns a map where keys are columns and values pre-processing algorithms
  std::map<QString, PreprocessingAlgorithm> asMap() const;

private:
  // A map where keys are columns and values pre-processing algorithms
  std::map<QString, PreprocessingAlgorithm> m_map;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt