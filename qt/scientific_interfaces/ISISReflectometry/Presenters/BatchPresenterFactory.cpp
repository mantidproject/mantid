#include "BatchPresenterFactory.h"
#include "MantidKernel/make_unique.h"

namespace MantidQt {
namespace CustomInterfaces {

BatchPresenterFactory::BatchPresenterFactory(
    std::vector<std::string> const &instruments, double thetaTolerance)
    : m_instruments(instruments), m_thetaTolerance(thetaTolerance) {}

std::unique_ptr<BatchPresenter> BatchPresenterFactory::
operator()(IBatchView *view) const {
  return Mantid::Kernel::make_unique<BatchPresenter>(
      view, m_instruments, m_thetaTolerance, UnslicedReductionJobs());
}
}
}
