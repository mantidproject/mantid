#ifndef INSTRUMENTWINDOW_H
#define INSTRUMENTWINDOW_H

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/AnalysisDataService.h"
#include <Mantid/IProjectSerialisable.h>

#include <MdiSubWindow.h>
#include <boost/shared_ptr.hpp>

class ApplicationWindow;
class InstrumentWidget;

using namespace Mantid;

class InstrumentWindow : public MdiSubWindow, public IProjectSerialisable {
  Q_OBJECT
public:
  explicit InstrumentWindow(const QString &wsName,
                            const QString &label = QString(),
                            ApplicationWindow *parent = nullptr,
                            const QString &name = QString());
  ~InstrumentWindow();

  void loadFromProject(const std::string &lines, ApplicationWindow *app,
                       const int fileVersion);
  std::string saveToProject(ApplicationWindow *app);
  InstrumentWidget *getInstrumentWidget() { return m_instrumentWidget; }
  void selectTab(int tab);

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
  InstrumentWidget *m_instrumentWidget;
};

#endif // INSTRUMENTWINDOW_H