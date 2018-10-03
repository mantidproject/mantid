// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ISISCALIBRATION_H_
#define MANTIDQTCUSTOMINTERFACES_ISISCALIBRATION_H_

#include "../General/UserInputValidator.h"
#include "IndirectDataReductionTab.h"
#include "MantidKernel/System.h"
#include "ui_ISISCalibration.h"

namespace MantidQt {
namespace CustomInterfaces {
/** ISISCalibration
  Handles vanadium run calibration for ISIS instruments.

  @author Dan Nixon
  @date 23/07/2014
*/
class DLLExport ISISCalibration : public IndirectDataReductionTab {
  Q_OBJECT

public:
  ISISCalibration(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~ISISCalibration() override;

  void setup() override;
  void run() override;
  bool validate() override;

  std::pair<double, double> peakRange() const;
  std::pair<double, double> backgroundRange() const;
  std::pair<double, double> resolutionRange() const;

  QString peakRangeString() const;
  QString backgroundRangeString() const;
  QString instrumentDetectorRangeString() const;
  QString outputWorkspaceName() const;
  QString resolutionDetectorRangeString() const;
  QString rebinString() const;
  QString backgroundString() const;

  void setPeakRange(const double &minimumTof, const double &maximumTof);
  void setBackgroundRange(const double &minimumTof, const double &maximumTof);
  void setPeakRangeLimits(const double &peakMin, const double &peakMax);
  void setBackgroundRangeLimits(const double &backgroundMin,
                                const double &backgroundMax);
  void setResolutionSpectraRange(const double &minimum, const double &maximum);

private slots:
  void algorithmComplete(bool error);
  void calPlotRaw();
  void calPlotEnergy();
  void calMinChanged(double);
  void calMaxChanged(double);
  void calUpdateRS(QtProperty *, double);
  void calSetDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws);
  void resCheck(bool state); ///< handles checking/unchecking of "Create RES
  /// File" checkbox
  void setDefaultInstDetails();
  void
  pbRunEditing(); //< Called when a user starts to type / edit the runs to load.
  void pbRunFinding();  //< Called when the FileFinder starts finding the files.
  void pbRunFinished(); //< Called when the FileFinder has finished finding the
  // files.
  /// Handles saving and plotting
  void saveClicked();
  void plotClicked();

private:
  void createRESfile(const QString &file);
  void addRuntimeSmoothing(const QString &workspaceName);
  void setRangeLimits(MantidWidgets::RangeSelector *rangeSelector,
                      const double &minimum, const double &maximum,
                      const QString &minPropertyName,
                      const QString &maxPropertyName);
  Mantid::API::IAlgorithm_sptr
  calibrationAlgorithm(const QString &inputFiles) const;
  Mantid::API::IAlgorithm_sptr
  resolutionAlgorithm(const QString &inputFiles) const;
  Mantid::API::IAlgorithm_sptr
  energyTransferReductionAlgorithm(const QString &inputFiles) const;

  Ui::ISISCalibration m_uiForm;
  QString m_lastCalPlotFilename;

  QString m_outputCalibrationName;
  QString m_outputResolutionName;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ISISCALIBRATION_H_
