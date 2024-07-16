// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataReductionTab.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "ui_ISISCalibration.h"

namespace MantidQt {
namespace CustomInterfaces {
class IDataReduction;

/** ISISCalibration
  Handles vanadium run calibration for ISIS instruments.

  @author Dan Nixon
  @date 23/07/2014
*/
class MANTIDQT_INDIRECT_DLL ISISCalibration : public DataReductionTab, public IRunSubscriber {
  Q_OBJECT

public:
  ISISCalibration(IDataReduction *idrUI, QWidget *parent = nullptr);
  ~ISISCalibration() override;

  std::pair<double, double> peakRange() const;
  std::pair<double, double> backgroundRange() const;
  std::pair<double, double> resolutionRange() const;

  QString peakRangeString() const;
  QString backgroundRangeString() const;
  QString instrumentDetectorRangeString();
  QString outputWorkspaceName() const;
  QString resolutionDetectorRangeString() const;
  QString rebinString() const;
  QString backgroundString() const;

  void setPeakRange(const double &minimumTof, const double &maximumTof);
  void setBackgroundRange(const double &minimumTof, const double &maximumTof);
  void setPeakRangeLimits(const double &peakMin, const double &peakMax);
  void setBackgroundRangeLimits(const double &backgroundMin, const double &backgroundMax);
  void setResolutionSpectraRange(const double &minimum, const double &maximum);

  void handleRun() override;
  void handleValidation(IUserInputValidator *validator) const override;

private slots:
  void algorithmComplete(bool error);
  void calPlotRaw();
  void calPlotEnergy();
  void calMinChanged(double /*val*/);
  void calMaxChanged(double /*val*/);
  void calUpdateRS(QtProperty * /*prop*/, double /*val*/);
  void calSetDefaultResolution(const Mantid::API::MatrixWorkspace_const_sptr &ws);
  void resCheck(bool state); ///< handles checking/unchecking of "Create RES
  /// File" checkbox
  void pbRunFinding();  //< Called when the FileFinder starts finding the files.
  void pbRunFinished(); //< Called when the FileFinder has finished finding the
  // files.
  /// Handles running, saving and plotting
  void saveClicked();

  void setSaveEnabled(bool enabled);

private:
  void updateInstrumentConfiguration() override;

  void setDefaultInstDetails(QMap<QString, QString> const &instrumentDetails);
  void connectRangeSelectors();
  void disconnectRangeSelectors();
  void addRuntimeSmoothing(const QString &workspaceName);
  void setRangeLimits(MantidWidgets::RangeSelector *rangeSelector, const double &minimum, const double &maximum,
                      const QString &minPropertyName, const QString &maxPropertyName);
  Mantid::API::IAlgorithm_sptr calibrationAlgorithm(const QString &inputFiles);
  Mantid::API::IAlgorithm_sptr resolutionAlgorithm(const QString &inputFiles) const;
  Mantid::API::IAlgorithm_sptr energyTransferReductionAlgorithm(const QString &inputFiles) const;

  Ui::ISISCalibration m_uiForm;
  QString m_lastCalPlotFilename;

  QString m_outputCalibrationName;
  QString m_outputResolutionName;
};
} // namespace CustomInterfaces
} // namespace MantidQt
