// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <optional>
#include <string>
#include <vector>

namespace MantidQt {

namespace MantidWidgets {
class PeakPicker;
class PreviewPlot;
} // namespace MantidWidgets

namespace CustomInterfaces {

class IALFAnalysisPresenter;

class MANTIDQT_DIRECT_DLL IALFAnalysisView {
public:
  virtual ~IALFAnalysisView() = default;

  virtual QWidget *getView() = 0;

  virtual void subscribePresenter(IALFAnalysisPresenter *presenter) = 0;

  virtual void replot() = 0;

  virtual std::pair<double, double> getRange() const = 0;

  virtual void addSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;
  virtual void addFitSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;
  virtual void removeFitSpectrum() = 0;

  virtual void setAverageTwoTheta(std::optional<double> average, std::vector<double> const &all) = 0;

  virtual void setPeak(Mantid::API::IPeakFunction_const_sptr const &peak) = 0;
  virtual Mantid::API::IPeakFunction_const_sptr getPeak() const = 0;

  virtual void setPeakCentre(double const centre) = 0;
  virtual double peakCentre() const = 0;
  virtual void setPeakCentreStatus(std::string const &status) = 0;

  virtual void setRotationAngle(std::optional<double> rotation) = 0;

  virtual void displayWarning(std::string const &message) = 0;
};

class MANTIDQT_DIRECT_DLL ALFAnalysisView final : public QWidget, public IALFAnalysisView {
  Q_OBJECT

public:
  explicit ALFAnalysisView(double const start, double const end, QWidget *parent = nullptr);

  QWidget *getView() override;

  void subscribePresenter(IALFAnalysisPresenter *presenter) override;

  void replot() override;

  std::pair<double, double> getRange() const override;

  void addSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void addFitSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void removeFitSpectrum() override;

  void setAverageTwoTheta(std::optional<double> average, std::vector<double> const &all) override;

  void setPeak(Mantid::API::IPeakFunction_const_sptr const &peak) override;
  Mantid::API::IPeakFunction_const_sptr getPeak() const override;

  void setPeakCentre(double const centre) override;
  double peakCentre() const override;
  void setPeakCentreStatus(std::string const &status) override;

  void setRotationAngle(std::optional<double> rotation) override;

  void displayWarning(std::string const &message) override;

private slots:
  void notifyPeakPickerChanged();
  void notifyPeakCentreEditingFinished();
  void notifyFitClicked();
  void notifyResetClicked();

private:
  void setupPlotFitSplitter(double const start, double const end);
  QWidget *createPlotWidget();
  QWidget *createPlotToolbar();
  QWidget *createFitWidget(double const start, double const end);
  void setupTwoThetaWidget(QGridLayout *layout);
  void setupFitRangeWidget(QGridLayout *layout, double const start, double const end);
  void setupPeakCentreWidget(QGridLayout *layout, double const centre);
  void setupRotationAngleWidget(QGridLayout *layout);

  MantidWidgets::PreviewPlot *m_plot;
  MantidWidgets::PeakPicker *m_peakPicker;
  QLineEdit *m_start, *m_end;
  QPushButton *m_fitButton;
  QPushButton *m_resetButton;
  QLineEdit *m_peakCentre;
  QLabel *m_fitStatus;
  QLineEdit *m_averageTwoTheta;
  QLabel *m_numberOfTubes;
  QLineEdit *m_rotationAngle;
  QLabel *m_fitRequired;

  IALFAnalysisPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
