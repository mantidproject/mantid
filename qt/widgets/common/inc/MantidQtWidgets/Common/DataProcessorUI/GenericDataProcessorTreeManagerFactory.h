#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorTreeManagerFactory.h"
namespace MantidQt {
namespace MantidWidgets {
class GenericDataProcessorTreeManagerFactory
    : public DataProcessorTreeManagerFactory {
  std::pair<std::unique_ptr<DataProcessorTreeManager>, DataProcessorTreeManagerFactory::PostProcessing>
  fromPostProcessorName(GenericDataProcessorPresenter &presenter,
                        const QString &postprocessorName,
                        DataProcessorWhiteList whitelist) override;
};
}
}
