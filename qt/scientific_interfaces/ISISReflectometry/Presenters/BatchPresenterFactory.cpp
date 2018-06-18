#include "BatchPresenterFactory.h"
#include "MantidKernel/make_unique.h"

namespace MantidQt {
namespace CustomInterfaces {

BatchPresenterFactory::BatchPresenterFactory(
    std::vector<std::string> const &instruments)
    : m_instruments(instruments) {}

std::unique_ptr<BatchPresenter> BatchPresenterFactory::
operator()(IBatchView *view) const {
  return Mantid::Kernel::make_unique<BatchPresenter>(view, m_instruments,
                                          UnslicedReductionJobs());
}

}
}
