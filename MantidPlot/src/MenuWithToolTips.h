// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MENU_WITH_TOOLTIPS
#define MENU_WITH_TOOLTIPS

#include <QMenu>

/**
 * Displays a tooltip for the menu item when it is set.
 */
class MenuWithToolTips : public QMenu {
  Q_OBJECT
public:
  explicit MenuWithToolTips(QWidget *parent = nullptr);
  MenuWithToolTips(const QString &title, QWidget *parent = nullptr);
  bool event(QEvent *e) override;
};

#endif // MENU_WITH_TOOLTIPS
