#ifndef MANTID_VATES_MD_LOADING_VIEW_SIMPLE_H
#define MANTID_VATES_MD_LOADING_VIEW_SIMPLE_H

#include "MantidVatesAPI/MDLoadingView.h"

namespace Mantid {
namespace VATES {

class DLLExport MDLoadingViewSimple : public MDLoadingView {
public:
  double getTime() const override;
  void setTime(double time);

  size_t getRecursionDepth() const override;
  void setRecursionDepth(size_t recursionDepth);

  bool getLoadInMemory() const override;
  void setLoadInMemory(bool loadInMemory);

private:
  double m_time = 0.0;
  size_t m_recursionDepth = 5;
  bool m_loadInMemory = true;
};

}
}

#endif
