#include "MantidQtWidgets/Common/DataProcessorUI/GenericCommandProviderFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericOneLevelCommandProvider.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericTwoLevelCommandProvider.h"
#include "MantidKernel/make_unique.h"
namespace MantidQt {
namespace MantidWidgets {
std::unique_ptr<DataProcessorCommandProvider>
GenericCommandProviderFactory::fromPostprocessorName(QString const& postprocessorName, GenericDataProcessorPresenter &presenter) const {
  if (postprocessorName.isEmpty()) 
    return ::Mantid::Kernel::make_unique<GenericOneLevelCommandProvider>(presenter);
  else 
    return ::Mantid::Kernel::make_unique<GenericTwoLevelCommandProvider>(presenter);
}
}
}
