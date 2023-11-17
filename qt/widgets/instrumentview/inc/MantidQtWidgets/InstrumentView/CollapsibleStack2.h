// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class CollapsingStack;
class CollapsingPanel;

class CollapsingPanelLabel : public QLabel {
  Q_OBJECT
public:
  CollapsingPanelLabel(const QString &caption, CollapsingPanel *parent);
  void mousePressEvent(QMouseEvent *e) override;
  void paintEvent(QPaintEvent *event) override;
signals:
  void collapseOrExpand();

private:
  CollapsingPanel *m_parentPanel;
};

class CollapsingPanel : public QWidget {
  Q_OBJECT
public:
  CollapsingPanel(const QString &caption, QWidget *parent);
  void setWidget(QWidget *widget, const bool fixedHeight);
  void setCaption(const QString &caption);
  bool isCollapsed() const;
  bool isFixed() const;
  QWidget *getWidget() const;
  CollapsingPanelLabel *getLabel() const;
  void collapseCaption();
  void expandCaption();
  void setFixedHeight(const int height);

signals:
  void collapsed();
  void expanded();
private slots:
  void collapsedOrExpanded();

private:
  QWidget *m_widget;
  QVBoxLayout *m_layout;
  CollapsingPanelLabel *m_label;
  bool m_isCollapsed;
  int m_maxHeight;
  int m_isFixed;
};

class CollapsingStack : public QWidget {
  Q_OBJECT
public:
  CollapsingStack(QWidget *parent);
  CollapsingPanel *addPanel(const QString &caption, QWidget *widget, const bool fixedHeight = false);
private slots:
  void panelCollapsed();
  void panelExpanded();

private:
  bool allCollapsedOrFixed() const;
  void updateStretch();
  QSplitter *m_splitterLayout;
  QVBoxLayout *m_baseLayout;
};
} // namespace MantidWidgets
} // namespace MantidQt
