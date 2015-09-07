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

  // Connect slots
  connect(m_uiForm.cbGeometry, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleGeometryChanged(int)));
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(newData(const QString &)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(plotPreview(int)));

  m_uiForm.spPreviewSpec->setMinimum(0);
  m_uiForm.spPreviewSpec->setMaximum(0);
}

void ContainerSubtraction::setup() {}
void ContainerSubtraction::run() {}
bool ContainerSubtraction::validate() { return false; }

void ContainerSubtraction::loadSettings(const QSettings &settings) {
  m_uiForm.dsContainer->readSettings(settings.group());
  m_uiForm.dsSample->readSettings(settings.group());
}
}
}