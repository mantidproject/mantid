#ifndef MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
#define MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
#include "DllConfig.h"
#include <memory>
#include <vector>
#include <string>
#include "BatchPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL BatchPresenterFactory {
public:
  BatchPresenterFactory(std::vector<std::string> const &instruments,
                        double thetaTolerance);
  std::unique_ptr<BatchPresenter> operator()(IBatchView *view) const;

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
