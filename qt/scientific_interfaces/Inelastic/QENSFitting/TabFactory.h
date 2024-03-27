// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QTabWidget>

namespace MantidQt::CustomInterfaces::IDA {

class Tab;

class TabFactory {
public:
  explicit TabFactory(QTabWidget *tabWidget);
  Tab *makeMSDFitTab(int const index) const;
  Tab *makeIqtFitTab(int const index) const;
  Tab *makeConvFitTab(int const index) const;
  Tab *makeFqFitTab(int const index) const;

private:
  QTabWidget *m_tabWidget;
};

} // namespace MantidQt::CustomInterfaces::IDA