// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTWIDGETTREETAB_H_
#define INSTRUMENTWIDGETTREETAB_H_

#include "InstrumentWidgetTab.h"

#include <QModelIndex>

namespace MantidQt {
namespace MantidWidgets {
class InstrumentTreeWidget;

/**
 * Implements the instrument tree tab in InstrumentWidget
 */
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidgetTreeTab
    : public InstrumentWidgetTab {
  Q_OBJECT
public:
  explicit InstrumentWidgetTreeTab(InstrumentWidget *instrWidget);
  void initSurface() override;
  /// Load settings for the tree widget tab from a project file
  virtual void loadFromProject(const std::string &lines) override;
  /// Save settings for the tree widget tab to a project file
  virtual std::string saveToProject() const override;
public slots:
  void selectComponentByName(const QString &name);

private:
  void showEvent(QShowEvent * /*unused*/) override;
  /// Widget to display instrument tree
  InstrumentTreeWidget *m_instrumentTree;
  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*INSTRUMENTWIDGETTREETAB_H_*/
