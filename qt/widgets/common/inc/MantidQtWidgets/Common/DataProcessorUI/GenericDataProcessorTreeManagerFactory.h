#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorTreeManagerFactory.h"
#include "MantidQtWidgets/Common/DllOption.h"
namespace MantidQt {
namespace MantidWidgets {
class EXPORT_OPT_MANTIDQT_COMMON GenericDataProcessorTreeManagerFactory
    : public DataProcessorTreeManagerFactory {
  std::pair<std::unique_ptr<DataProcessorTreeManager>,
            DataProcessorTreeManagerFactory::PostProcessing>
  fromPostProcessorName(GenericDataProcessorPresenter &presenter,
                        const QString &postprocessorName,
                        DataProcessorWhiteList whitelist) override;
};
}
}
