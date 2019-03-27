// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTWIDGETTAB_H
#define INSTRUMENTWIDGETTAB_H

#include "InstrumentWidgetTypes.h"

#include <QFrame>
#include <boost/shared_ptr.hpp>

//--------------------------------------------------
//  Forward declarations
//--------------------------------------------------

class QSettings;
class QMenu;

namespace MantidQt {
namespace MantidWidgets {
class InstrumentWidget;
class ProjectionSurface;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidgetTab
    : public QFrame,
      public InstrumentWidgetTypes {
  Q_OBJECT
public:
  explicit InstrumentWidgetTab(InstrumentWidget *parent);
  /// Called by InstrumentWidget after the projection surface crated
  /// Use it for surface-specific initialization
  virtual void initSurface() {}
  /// Save tab's persistent settings to the provided QSettings instance
  virtual void saveSettings(QSettings & /*unused*/) const {}
  /// Load (read and apply) tab's persistent settings from the provided
  /// QSettings instance
  virtual void loadSettings(const QSettings & /*unused*/) {}
  /// Add tab-specific items to the context menu
  /// Return true if at least 1 item was added or false otherwise.
  virtual bool addToDisplayContextMenu(QMenu & /*unused*/) const { return false; }
  /// Get the projection surface
  boost::shared_ptr<ProjectionSurface> getSurface() const;
  /// Load state for the widget tab from a project file
  virtual void loadFromProject(const std::string &lines) = 0;
  /// Save state for the widget tab to a project file
  virtual std::string saveToProject() const = 0;

protected:
  /// The parent InstrumentWidget
  InstrumentWidget *m_instrWidget;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // INSTRUMENTWIDGETTAB_H
