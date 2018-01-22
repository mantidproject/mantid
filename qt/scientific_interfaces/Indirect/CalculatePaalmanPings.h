#ifndef MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_

#include "CorrectionsTab.h"
#include "ui_CalculatePaalmanPings.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport CalculatePaalmanPings : public CorrectionsTab {
  Q_OBJECT

public:
  CalculatePaalmanPings(QWidget *parent = nullptr);

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

  bool doValidation(bool silent = false);

private slots:
  void absCorComplete(bool error);
  void postProcessComplete(bool error);
  void getBeamWidthFromWorkspace(const QString &wsName);
  void fillCorrectionDetails(const QString &wsName);
  void validateChemical();
  void saveClicked();
  void plotClicked();
  void changeSampleDensityUnit(int);
  void changeCanDensityUnit(int);

private:
  void addShapeSpecificSampleOptions(Mantid::API::IAlgorithm_sptr alg,
                                     QString shape);
  void addShapeSpecificCanOptions(Mantid::API::IAlgorithm_sptr alg,
                                  QString shape);
  boost::optional<double>
  getInstrumentParameter(Mantid::Geometry::Instrument_const_sptr instrument,
                         const std::string &parameterName);

  Ui::CalculatePaalmanPings m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_ */
