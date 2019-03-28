// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_

#include "CorrectionsTab.h"
#include "ui_CalculatePaalmanPings.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

struct Densities {
  Densities() : m_massDensity(1.0), m_numberDensity(0.1){};

  void setMassDensity(double value) { m_massDensity = value; }
  void setNumberDensity(double value) { m_numberDensity = value; }
  double getMassDensity() const { return m_massDensity; }
  double getNumberDensity() const { return m_numberDensity; }

private:
  double m_massDensity;
  double m_numberDensity;
};

class DLLExport CalculatePaalmanPings : public CorrectionsTab {
  Q_OBJECT

public:
  CalculatePaalmanPings(QWidget *parent = nullptr);

private slots:
  void absCorComplete(bool error);
  void postProcessComplete(bool error);
  void getBeamWidthFromWorkspace(const QString &wsName);
  void fillCorrectionDetails(const QString &wsName);
  void validateChemical();
  void saveClicked();
  void plotClicked();
  void runClicked();
  void setSampleDensityUnit(int);
  void setCanDensityUnit(int);
  void setSampleDensityValue(int index);
  void setCanDensityValue(int index);
  void changeSampleMaterialOptions(int index);
  void changeCanMaterialOptions(int index);
  void setSampleDensity(double value);
  void setCanDensity(double value);

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

  bool doValidation(bool silent = false);

  void addShapeSpecificSampleOptions(Mantid::API::IAlgorithm_sptr alg,
                                     QString shape);
  void addShapeSpecificCanOptions(Mantid::API::IAlgorithm_sptr alg,
                                  QString shape);

  void setRunEnabled(bool enabled);
  void setPlotResultEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);
  void setPlotResultIsPlotting(bool plotting);

  boost::optional<double>
  getInstrumentParameter(Mantid::Geometry::Instrument_const_sptr instrument,
                         const std::string &parameterName);

  std::shared_ptr<Densities> m_sampleDensities;
  std::shared_ptr<Densities> m_canDensities;

  Ui::CalculatePaalmanPings m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CALCULATEPAALMANPINGS_H_ */
