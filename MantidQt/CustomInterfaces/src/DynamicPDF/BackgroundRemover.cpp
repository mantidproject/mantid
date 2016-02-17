#include "MantidQtCustomInterfaces/DynamicPDF/BackgroundRemover.h"
#include "MantidQtCustomInterfaces/DynamicPDF/SliceSelector.h"

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

BackgroundRemover::BackgroundRemover(SliceSelector *parent) : QDialog(parent) {
  m_uiForm.setupUi(this);
}

void BackgroundRemover::refreshSlice(
    const boost::shared_ptr<WorkspaceRecord> loadedWorkspace,
    const size_t &workspaceIndex) {
  UNUSED_ARG(loadedWorkspace);
  UNUSED_ARG(workspaceIndex);
  std::cout << "hello world";
}
}
}
}
