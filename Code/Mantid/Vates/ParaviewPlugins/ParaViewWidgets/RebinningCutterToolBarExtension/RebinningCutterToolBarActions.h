#include <QActionGroup>
#include <pqPipelineSource.h>
#include "MantidKernel/System.h"

class DLLExport RebinningCutterToolBarActions : public QActionGroup
{
  Q_OBJECT
public:
  RebinningCutterToolBarActions(QObject* p);
  ~RebinningCutterToolBarActions();

public slots:
  void createTargetFilter();

private:

  pqPipelineSource *getActiveSource() const;

};

