#ifndef INSTRUMENTWINDOW_H
#define INSTRUMENTWINDOW_H

#include <Mantid/IProjectSerialisable.h>
#include <InstrumentWidget.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmObserver.h"

#include <boost/shared_ptr.hpp>

class ApplicationWindow;
class MdiSubWindow;

using namespace Mantid;

class InstrumentWindow : public InstrumentWidget,
                               public IProjectSerialisable {
  Q_OBJECT
public:
  explicit InstrumentWindow(MdiSubWindow *parent, const QString &wsName);
  ~InstrumentWindow();

  void loadFromProject(const std::string &lines, ApplicationWindow *app,
                       const int fileVersion);
  std::string saveToProject(ApplicationWindow *app);

protected:
	virtual void closeEvent(QCloseEvent *e);
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
  MdiSubWindow *m_mdiSubWindowParent;
};

#endif // INSTRUMENTWINDOW_H