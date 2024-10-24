// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <QWidget>
#include <array>
#include <cstddef>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
template <std::size_t N> class QWidgetGroup {
public:
  QWidgetGroup() : m_widgets() {}
  explicit QWidgetGroup(std::array<QWidget *, N> const &widgets) : m_widgets(widgets) {}

  void enable() {
    for (auto *widget : m_widgets)
      widget->setEnabled(true);
  }

  void disable() {
    for (auto *widget : m_widgets)
      widget->setEnabled(false);
  }

private:
  std::array<QWidget *, N> m_widgets;
};

template <typename... Ts> QWidgetGroup<sizeof...(Ts)> makeQWidgetGroup(Ts... widgets) {
  return QWidgetGroup<sizeof...(Ts)>(std::array<QWidget *, sizeof...(Ts)>({{widgets...}}));
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
