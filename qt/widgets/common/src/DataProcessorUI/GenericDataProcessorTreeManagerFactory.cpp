#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorTreeManagerFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorOneLevelTreeManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorTwoLevelTreeManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"
namespace MantidQt {
namespace MantidWidgets {
std::pair<std::unique_ptr<DataProcessorTreeManager>, DataProcessorTreeManagerFactory::PostProcessing>
GenericDataProcessorTreeManagerFactory::fromPostProcessorName(
    GenericDataProcessorPresenter &presenter, QString const &postprocessorName,
    DataProcessorWhiteList whitelist) {
  if (postprocessorName.isEmpty()) {
    return std::make_pair(
        Mantid::Kernel::make_unique<DataProcessorOneLevelTreeManager>(
            &presenter, whitelist),
        DataProcessorTreeManagerFactory::PostProcessing::Yes);
  } else {
    return std::make_pair(
        Mantid::Kernel::make_unique<DataProcessorTwoLevelTreeManager>(
            &presenter, whitelist),
        DataProcessorTreeManagerFactory::PostProcessing::No);
  }
}
}
}
