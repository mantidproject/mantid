#include <QActionGroup>
#include <pqPipelineSource.h>

class RebinningCutterToolBarActions : public QActionGroup
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

