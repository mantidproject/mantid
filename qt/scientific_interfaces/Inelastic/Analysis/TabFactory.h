// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QTabWidget>

namespace MantidQt::CustomInterfaces::IDA {

class DataAnalysisTab;

class TabFactory {
public:
  explicit TabFactory(QTabWidget *tabWidget);
  DataAnalysisTab *makeMSDFitTab(int const index) const;
  DataAnalysisTab *makeIqtFitTab(int const index) const;
  DataAnalysisTab *makeConvFitTab(int const index) const;
  DataAnalysisTab *makeFqFitTab(int const index) const;

private:
  QTabWidget *m_tabWidget;
};

} // namespace MantidQt::CustomInterfaces::IDA