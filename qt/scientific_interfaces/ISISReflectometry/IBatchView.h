#ifndef MANTID_CUSTOMINTERFACES_IBATCHVIEW_H_
#define MANTID_CUSTOMINTERFACES_IBATCHVIEW_H_
#include "DllConfig.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"

namespace MantidQt {
namespace CustomInterfaces {

class BatchViewSubscriber
    : public MantidQt::MantidWidgets::Batch::JobTreeViewSubscriber {
public:
  virtual void notifyProcessRequested() = 0;
  virtual void notifyPauseRequested() = 0;
  virtual void notifyInsertRowRequested() = 0;
  virtual void notifyInsertGroupRequested() = 0;
  virtual void notifyDeleteRowRequested() = 0;
  virtual void notifyDeleteGroupRequested() = 0;
  virtual void notifyFilterChanged(std::string const& filterValue) = 0;

  virtual void notifyExpandAllRequested() = 0;
  virtual void notifyCollapseAllRequested() = 0;
  virtual ~BatchViewSubscriber() = default;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IBatchView {
public:
  virtual void subscribe(BatchViewSubscriber *notifyee) = 0;
  virtual void setProgress(int value) = 0;
  virtual void resetFilterBox() = 0;
  virtual MantidQt::MantidWidgets::Batch::IJobTreeView &jobs() = 0;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_IBATCHVIEW_H_
