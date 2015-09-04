#include "MantidQtCustomInterfaces/Indirect/ContainerSubtraction.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ContainerSubtraction");
}

namespace MantidQt {
namespace CustomInterfaces {
ContainerSubtraction::ContainerSubtraction(QWidget *parent)
    : CorrectionsTab(parent) {
  m_uiForm.setupUi(parent);
}

void ContainerSubtraction::setup() {}
void ContainerSubtraction::run() {}
bool ContainerSubtraction::validate() { return false; }

void ContainerSubtraction::loadSettings(const QSettings &settings) {
  m_uiForm.dsCorrections->readSettings(settings.group());
  m_uiForm.dsContainer->readSettings(settings.group());
  m_uiForm.dsSample->readSettings(settings.group());
}
}
}