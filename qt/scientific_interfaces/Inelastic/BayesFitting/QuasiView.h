// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_Quasi.h"

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <string>
#include <utility>

#include <QMap>
#include <QObject>
#include <QString>
#include <QWidget>

class DoubleEditorFactory;
class QtDoublePropertyManager;
class QtProperty;
class QtTreePropertyBrowser;

namespace MantidQt::CustomInterfaces {
class IRunView;
class IQuasiPresenter;

class MANTIDQT_INELASTIC_DLL IQuasiView {
public:
  virtual ~IQuasiView() = default;

  virtual void subscribe(IQuasiPresenter *presenter) = 0;
  virtual IRunView *getRunView() const = 0;
  virtual MantidWidgets::DataSelector *sampleSelector() const = 0;
  virtual MantidWidgets::DataSelector *resolutionSelector() const = 0;
  virtual MantidWidgets::DataSelector *resNormSelector() const = 0;
  virtual API::FileFinderWidget *fixWidthFileFinder() const = 0;

  virtual void setPreviewSpectrumMax(std::size_t const max) = 0;
  virtual void setXRange(std::pair<double, double> const &range) = 0;

  virtual void watchADS(bool const watch) = 0;
  virtual void clearPlot() = 0;
  virtual bool hasSpectrum(std::string const &label) const = 0;
  virtual void addSpectrum(std::string const &label, Mantid::API::MatrixWorkspace_sptr const &workspace,
                           std::size_t const spectrumIndex, std::string const &colour = "") = 0;
  virtual std::size_t previewSpectrum() const = 0;

  virtual std::string sampleName() const = 0;
  virtual std::string resolutionName() const = 0;
  virtual std::string resNormName() const = 0;
  virtual std::string fixWidthName() const = 0;
  virtual std::string programName() const = 0;
  virtual std::string backgroundName() const = 0;
  virtual std::string plotName() const = 0;

  virtual double eMin() const = 0;
  virtual double eMax() const = 0;

  virtual int sampleBinning() const = 0;
  virtual int resolutionBinning() const = 0;

  virtual bool useResolution() const = 0;
  virtual bool fixWidth() const = 0;
  virtual bool elasticPeak() const = 0;
  virtual bool sequentialFit() const = 0;

  virtual void setPlotResultEnabled(bool const enable) = 0;
  virtual void setSaveResultEnabled(bool const enable) = 0;

  virtual void enableUseResolution(bool const enable) = 0;
  virtual void enableView(bool const enable) = 0;

  virtual bool displaySaveDirectoryMessage() const = 0;

  virtual void setFileExtensionsByName(bool const filter) = 0;
  virtual void setLoadHistory(bool const loadHistory) = 0;

  virtual void loadSettings(const QSettings &settings) = 0;
};

class QuasiView final : public QWidget, public IQuasiView {
  Q_OBJECT

public:
  QuasiView(QWidget *parent = nullptr);
  ~QuasiView() override = default;

  void subscribe(IQuasiPresenter *presenter) override;
  IRunView *getRunView() const override;
  MantidWidgets::DataSelector *sampleSelector() const override;
  MantidWidgets::DataSelector *resolutionSelector() const override;
  MantidWidgets::DataSelector *resNormSelector() const override;
  API::FileFinderWidget *fixWidthFileFinder() const override;

  void setPreviewSpectrumMax(std::size_t const max) override;
  void setXRange(std::pair<double, double> const &range) override;

  void watchADS(bool const watch) override;
  void clearPlot() override;
  bool hasSpectrum(std::string const &label) const override;
  void addSpectrum(std::string const &label, Mantid::API::MatrixWorkspace_sptr const &workspace,
                   std::size_t const spectrumIndex, std::string const &colour = "") override;
  std::size_t previewSpectrum() const override;

  std::string sampleName() const override;
  std::string resolutionName() const override;
  std::string resNormName() const override;
  std::string fixWidthName() const override;
  std::string programName() const override;
  std::string backgroundName() const override;
  std::string plotName() const override;

  double eMin() const override;
  double eMax() const override;

  int sampleBinning() const override;
  int resolutionBinning() const override;

  bool useResolution() const override;
  bool fixWidth() const override;
  bool elasticPeak() const override;
  bool sequentialFit() const override;

  void setPlotResultEnabled(bool const enable) override;
  void setSaveResultEnabled(bool const enable) override;

  void enableUseResolution(bool const enable) override;
  void enableView(bool const enable) override;

  bool displaySaveDirectoryMessage() const override;

  void setFileExtensionsByName(bool const filter) override;
  void setLoadHistory(bool const loadHistory) override;

  void loadSettings(const QSettings &settings) override;

private slots:
  void minEValueChanged(double const min);
  void maxEValueChanged(double const max);
  void updateProperties(QtProperty *prop, double const value);

  void handleProgramChange(int const index);

  void notifySampleInputReady(QString const &workspaceName);
  void notifyResolutionInputReady(QString const &workspaceName);
  void notifyFileAutoLoaded();

  void notifyPreviewSpectrumChanged(int const value);

  void notifyPlotCurrentPreview();
  void notifySaveClicked();
  void notifyPlotClicked();

private:
  void setupFitOptions();
  void setupPropertyBrowser();
  void setupPlotOptions();

  Ui::Quasi m_uiForm;

  QtDoublePropertyManager *m_dblManager;
  QtTreePropertyBrowser *m_propTree;
  QMap<QString, QtProperty *> m_properties;
  DoubleEditorFactory *m_dblEditorFactory;

  IQuasiPresenter *m_presenter;
};
} // namespace MantidQt::CustomInterfaces
