// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_QWIDGETGROUP_H
#define MANTID_ISISREFLECTOMETRY_QWIDGETGROUP_H
#include <QWidget>
#include <array>
#include <cstddef>
namespace MantidQt {
namespace CustomInterfaces {
template <std::size_t N> class QWidgetGroup {
public:
  QWidgetGroup() : m_widgets() {}
  explicit QWidgetGroup(std::array<QWidget *, N> const &widgets)
      : m_widgets(widgets) {}

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

template <typename... Ts>
QWidgetGroup<sizeof...(Ts)> makeQWidgetGroup(Ts... widgets) {
  return QWidgetGroup<sizeof...(Ts)>(
      std::array<QWidget *, sizeof...(Ts)>({{widgets...}}));
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_QWIDGETGROUP_H
