#ifndef MANTID_CUSTOMINTERFACES_MOCKBATCHVIEW_H_
#define MANTID_CUSTOMINTERFACES_MOCKBATCHVIEW_H_
#include "DllConfig.h"
#include "IBatchView.h"
#include <gmock/gmock.h>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL MockBatchView : public IBatchView {
public:
  MOCK_METHOD1(subscribe, void(BatchViewSubscriber*));
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD0(jobs, IJobTreeView&());
};

}
}
#endif // MANTID_CUSTOMINTERFACES_MOCKBATCHVIEW_H_
