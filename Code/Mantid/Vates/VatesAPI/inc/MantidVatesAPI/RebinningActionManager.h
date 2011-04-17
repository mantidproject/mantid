/*
 * RebinningActionManager.h
 *
 *  Created on: 17 Apr 2011
 *      Author: owen
 */

#ifndef REBINNINGACTIONMANAGER_H_
#define REBINNINGACTIONMANAGER_H_

#include "MantidVatesAPI/Common.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace VATES
{
class DLLExport RebinningActionManager
{
public:
  virtual void ask(RebinningIterationAction requestedAction) = 0;
  virtual RebinningIterationAction action() const = 0;
  virtual void reset() = 0;
  virtual ~RebinningActionManager()
  {
  }
};
}
}

#endif /* REBINNINGACTIONMANAGER_H_ */
