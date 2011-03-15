#include "MantidVatesAPI/ProgressAction.h"

#ifndef UPDATEHANDLER_H_
#define UPDATEHANDLER_H_

class avtRebinningCutterFilter;
namespace Mantid
{
namespace VATES
{

class VisITProgressAction : public ProgressAction
{

public:

  VisITProgressAction(avtRebinningCutterFilter* filter);

  virtual void eventRaised(int progressPercent);

private:

  avtRebinningCutterFilter* m_filter;
};

}
}

#endif /* UPDATEHANDLER_H_ */
