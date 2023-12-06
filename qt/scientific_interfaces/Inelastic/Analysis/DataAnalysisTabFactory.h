// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QTabWidget>

namespace MantidQt::CustomInterfaces::IDA {

class IndirectDataAnalysisTab;

class DataAnalysisTabFactory {
public:
  explicit DataAnalysisTabFactory(QTabWidget *tabWidget);
  IndirectDataAnalysisTab *makeMSDFitTab(int const index) const;
  IndirectDataAnalysisTab *makeIqtFitTab(int const index) const;
  IndirectDataAnalysisTab *makeConvFitTab(int const index) const;
  IndirectDataAnalysisTab *makeFqFitTab(int const index) const;

private:
  QTabWidget *m_tabWidget;
};

} // namespace MantidQt::CustomInterfaces::IDA