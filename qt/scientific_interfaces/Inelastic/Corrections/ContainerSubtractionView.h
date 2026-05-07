// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ui_ContainerSubtraction.h"
#include <MantidQtWidgets/Common/UserInputValidator.h>
#include <MantidQtWidgets/Spectroscopy/OutputWidget/OutputNameView.h>
#include <QObject>
#include <QWidget>

namespace MantidQt::CustomInterfaces {
enum class CSCurves { SAMPLE, CONTAINER, SUBTRACTED };

class IContainerSubtractionPresenter;

class IContainerSubtractionView {
public:
  virtual ~IContainerSubtractionView() = default;

  virtual void subscribe(IContainerSubtractionPresenter *presenter) = 0;
  virtual void validate(IUserInputValidator *validator) = 0;
  virtual void setSampleWSSuffixes(const QStringList &suffixes) = 0;
  virtual void setSampleFBSuffixes(const QStringList &suffixes) = 0;
  virtual void setCanWSSuffixes(const QStringList &suffixes) = 0;
  virtual void setCanFBSuffixes(const QStringList &suffixes) = 0;

  virtual void loadSettings(const QSettings &settings) = 0;
  virtual void setLoadHistory(bool doLoadHistory) = 0;
  virtual void enableSaveButton(bool enable) = 0;
  virtual double getShift() const = 0;
  virtual double getScale() const = 0;
  virtual int getSpNo() const = 0;
  virtual int getSpMax() const = 0;
  virtual void setSpMax(int max) = 0;
  virtual void clearPlot() const = 0;
  virtual void plotSpectrum(const CSCurves &curveName, const MatrixWorkspace_sptr &ws, size_t specNo) = 0;
  virtual bool requestRebinToSample() = 0;

  virtual IOutputPlotOptionsView *getPlotOptions() const = 0;
  virtual IRunView *getRunView() const = 0;
  virtual IOutputNameView *getOutputNameView() const = 0;
};

class MANTIDQT_INELASTIC_DLL ContainerSubtractionView final : public QWidget, public IContainerSubtractionView {
  Q_OBJECT

public:
  explicit ContainerSubtractionView(QWidget *parent = nullptr);
  ~ContainerSubtractionView() override = default;

  void subscribe(IContainerSubtractionPresenter *presenter) override;
  void validate(IUserInputValidator *validator) override;

  void setSampleWSSuffixes(const QStringList &suffixes) override;
  void setSampleFBSuffixes(const QStringList &suffixes) override;
  void setCanWSSuffixes(const QStringList &suffixes) override;
  void setCanFBSuffixes(const QStringList &suffixes) override;

  void loadSettings(const QSettings &settings) override;
  void setLoadHistory(bool doLoadHistory) override;
  void enableSaveButton(bool enable) override;
  double getScale() const override;
  double getShift() const override;
  int getSpNo() const override;
  int getSpMax() const override;
  void setSpMax(int max) override;
  void clearPlot() const override;
  bool requestRebinToSample() override;

  IOutputPlotOptionsView *getPlotOptions() const override;
  IRunView *getRunView() const override;
  IOutputNameView *getOutputNameView() const override;

  void plotSpectrum(const CSCurves &curveName, const MatrixWorkspace_sptr &ws, size_t specNo) override;

private slots:
  void notifySampleDataReady(const QString &dataName) const;
  void notifyCanDataReady(const QString &dataName) const;
  void notifySpectraIncreaseClicked(int specNo) const;
  void notifyPreviewClicked() const;
  void notifySaveClicked() const;
  void notifyUpdateCan() const;

private:
  Ui::ContainerSubtraction m_uiForm;
  IContainerSubtractionPresenter *m_presenter;
};
} // namespace MantidQt::CustomInterfaces
