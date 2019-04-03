// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef COLLAPSIBLEPANEL_H_
#define COLLAPSIBLEPANEL_H_

#include <QLabel>
#include <QWidget>

class QVBoxLayout;

namespace MantidQt {
namespace MantidWidgets {
class CaptionLabel : public QLabel {
  Q_OBJECT
public:
  CaptionLabel(const QString &caption, QWidget *parent);
  void mousePressEvent(QMouseEvent *e) override;
  void paintEvent(QPaintEvent *event) override;
  bool isCollapsed() const { return m_collapsed; }
  void collapse();
  void expand();
signals:
  void collapseOrExpand(bool /*_t1*/);

private:
  bool m_collapsed;
};

/**
 * Implements a collapsible panel.
 */
class CollapsiblePanel : public QWidget {
  Q_OBJECT
public:
  CollapsiblePanel(const QString &caption, QWidget *parent);
  void setWidget(QWidget *widget);
  void setCaption(const QString &caption);
  bool isCollapsed() const;
signals:
  void collapsed();
  void expanded();
public slots:
  void collapse();
  void expand();
private slots:
  void collapseOrExpand(bool /*collapse*/);

private:
  QWidget *m_widget;
  QVBoxLayout *m_layout;
  CaptionLabel *m_label;
};

class CollapsibleStack : public QWidget {
  Q_OBJECT
public:
  explicit CollapsibleStack(QWidget *parent);
  CollapsiblePanel *addPanel(const QString &caption, QWidget *widget);
private slots:
  void updateStretch();

private:
  bool allCollapsed() const;
  QVBoxLayout *m_layout;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*COLLAPSIBLEPANEL_H_*/
