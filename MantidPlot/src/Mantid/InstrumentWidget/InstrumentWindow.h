// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTWINDOW_H
#define INSTRUMENTWINDOW_H

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtWidgets/Common/GraphOptions.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"

#include <MdiSubWindow.h>
#include <boost/shared_ptr.hpp>

class ApplicationWindow;
class MantidUI;

namespace MantidQt {
namespace MantidWidgets {
class InstrumentWidget;
class InstrumentWidgetTab;
} // namespace MantidWidgets
} // namespace MantidQt

class InstrumentWindow : public MdiSubWindow {
  Q_OBJECT
public:
  explicit InstrumentWindow(const QString &wsName,
                            const QString &label = QString(),
                            ApplicationWindow *parent = nullptr,
                            const QString &name = QString());
  ~InstrumentWindow() override;

  /// Load the state of the instrument window for a Mantid project file
  static MantidQt::API::IProjectSerialisable *
  loadFromProject(const std::string &lines, ApplicationWindow *app,
                  const int fileVersion);
  /// Returns a list of workspace names that are used by this window
  std::vector<std::string> getWorkspaceNames() override;
  /// Returns the user friendly name of the window
  std::string getWindowName() override;

  /// Save the state of the instrument window to a Mantid project file
  std::string saveToProject(ApplicationWindow *app) override;
  void selectTab(int tab);
  MantidQt::MantidWidgets::InstrumentWidgetTab *
  getTab(const QString &title) const;
  MantidQt::MantidWidgets::InstrumentWidgetTab *getTab(int tab) const;
  void setBinRange(double min_value, double max_value);
  bool overlay(const QString &wsName);
  void changeColormap();
  void changeColormap(const QString &file);
  void setColorMapMinValue(double);
  void setColorMapMaxValue(double);
  void setColorMapRange(double, double);
  void selectComponent(const QString &);
  void setScaleType(GraphOptions::ScaleType);
  void setViewType(const QString &);
  /// Get the window type as a string
  std::string getWindowType() override { return "Instrument"; }

public slots:
  void closeSafely();

private:
  MantidQt::MantidWidgets::InstrumentWidget *m_instrumentWidget;
};

#endif // INSTRUMENTWINDOW_H
