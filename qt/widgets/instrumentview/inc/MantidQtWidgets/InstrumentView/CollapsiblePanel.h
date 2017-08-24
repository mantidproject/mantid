#ifndef COLLAPSIBLEPANEL_H_
#define COLLAPSIBLEPANEL_H_

#include <QWidget>
#include <QLabel>

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
  void collapseOrExpand(bool);

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
  bool isCollapsed() const;
signals:
  void collapsed();
  void expanded();
public slots:
  void collapse();
  void expand();
  void setCaption(const QString &caption);
private slots:
  void collapseOrExpand(bool);

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

} // MantidWidgets
} // MantidQt

#endif /*COLLAPSIBLEPANEL_H_*/
