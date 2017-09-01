#include "MantidQtWidgets/Common/DataProcessorUI/GenericCommandProviderFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericOneLevelCommandProvider.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericTwoLevelCommandProvider.h"
namespace MantidQt {
namespace MantidWidgets {
std::unique_ptr<DataProcessorCommandProvider>
GenericCommandProviderFactory::fromPostprocessorName(QString const& postprocessorName, GenericDataProcessorPresenter &presenter) const {
  if (postprocessorName.isEmpty()) 
    return std::make_unique<GenericOneLevelCommandProvider>(presenter);
  else 
    return std::make_unique<GenericTwoLevelCommandProvider>(presenter);
}
}
}
