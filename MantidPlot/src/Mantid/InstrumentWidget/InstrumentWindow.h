#ifndef INSTRUMENTWINDOW_H
#define INSTRUMENTWINDOW_H

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/AnalysisDataService.h"
#include <Mantid/IProjectSerialisable.h>
#include <MantidQtAPI/GraphOptions.h>

#include <MdiSubWindow.h>
#include <boost/shared_ptr.hpp>

class ApplicationWindow;
namespace MantidQt {
namespace MantidWidgets {
class InstrumentWidget;
class InstrumentWidgetTab;
}
}

using namespace Mantid;

class InstrumentWindow : public MdiSubWindow, public IProjectSerialisable {
  Q_OBJECT
public:
  explicit InstrumentWindow(const QString &wsName,
                            const QString &label = QString(),
                            ApplicationWindow *parent = nullptr,
                            const QString &name = QString());
  ~InstrumentWindow() override;

  void loadFromProject(const std::string &lines, ApplicationWindow *app,
                       const int fileVersion) override;
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

private:
  /// ADS notification handlers
  virtual void preDeleteHandle(
      const std::string &ws_name,
      const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr);
  virtual void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr);
  virtual void renameHandle(const std::string &oldName,
                            const std::string &newName);
  virtual void clearADSHandle();

private:
  MantidQt::MantidWidgets::InstrumentWidget *m_instrumentWidget;
};

#endif // INSTRUMENTWINDOW_H