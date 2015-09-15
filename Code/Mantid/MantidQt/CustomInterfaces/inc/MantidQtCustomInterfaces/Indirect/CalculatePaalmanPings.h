#ifndef MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_

#include "ui_CalculatePaalmanPings.h"
#include "CorrectionsTab.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport CalculatePaalmanPings : public CorrectionsTab {
  Q_OBJECT

public:
  CalculatePaalmanPings(QWidget *parent = 0);

private:
  virtual void setup();
  virtual void run();
  virtual bool validate();
  virtual void loadSettings(const QSettings &settings);

  bool doValidation(bool silent = false);

private slots:
  void absCorComplete(bool error);
  void postProcessComplete(bool error);
  void getBeamWidthFromWorkspace(const QString &wsName);

private:
  void addShapeSpecificSampleOptions(Mantid::API::IAlgorithm_sptr alg,
                                     QString shape);
  void addShapeSpecificCanOptions(Mantid::API::IAlgorithm_sptr alg,
                                  QString shape);

  Ui::CalculatePaalmanPings m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_ */
