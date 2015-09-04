#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_

#include "ui_ContainerSubtraction.h"
#include "CorrectionsTab.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport ContainerSubtraction : public CorrectionsTab {
  Q_OBJECT

public:
  ContainerSubtraction(QWidget *parent = 0);

private slots:

private:
  virtual void setup();
  virtual void run();
  virtual bool validate();
  virtual void loadSettings(const QSettings &settings);

  Ui::ContainerSubtraction m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_ */