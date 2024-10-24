// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QTabWidget>

namespace MantidQt::CustomInterfaces::Inelastic {

class FitTab;

class TabFactory {
public:
  explicit TabFactory(QTabWidget *tabWidget);
  FitTab *makeMSDTab(int const index) const;
  FitTab *makeIqtTab(int const index) const;
  FitTab *makeConvolutionTab(int const index) const;
  FitTab *makeFunctionQTab(int const index) const;

private:
  QTabWidget *m_tabWidget;
};

} // namespace MantidQt::CustomInterfaces::Inelastic
