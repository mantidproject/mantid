// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ui_InstrumentConfig.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/InstrumentSelector.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class IInstrumentConfig {
public:
  virtual ~IInstrumentConfig() = default;

  virtual QStringList getTechniques() = 0;
  virtual void setTechniques(const QStringList &techniques) = 0;

  virtual QStringList getDisabledInstruments() = 0;
  virtual void setDisabledInstruments(const QStringList &instrumentNames) = 0;

  virtual QString getFacility() = 0;
  virtual void setFacility(const QString &facilityName) = 0;

  virtual bool isDiffractionEnabled() = 0;
  virtual void enableDiffraction(bool enabled) = 0;

  virtual bool isDiffractionForced() = 0;
  virtual void forceDiffraction(bool forced) = 0;

  virtual bool isInstrumentLabelShown() = 0;
  virtual void setShowInstrumentLabel(bool visible) = 0;

  virtual QString getInstrumentName() = 0;
  virtual void setInstrument(const QString &instrumentName) = 0;

  virtual QString getAnalyserName() = 0;
  virtual void setAnalyser(const QString &analyserName) = 0;

  virtual QString getReflectionName() = 0;
  virtual void setReflection(const QString &reflectionName) = 0;

  virtual void showAnalyserAndReflectionOptions(bool visible) = 0;
};

/**
Widget used to select an instrument configuration for indirect geometry
spectrometers
(i.e. and instrument, analyser and reflection).

Instruments are populated using an InstrumentSelector widget, analysers and
reflections
are populated by loading an empty instrument.

@author Dan Nixon
*/

class MANTIDQT_INDIRECT_DLL InstrumentConfig : public API::MantidWidget, public IInstrumentConfig {
  Q_OBJECT

  Q_PROPERTY(QStringList techniques READ getTechniques WRITE setTechniques)
  Q_PROPERTY(QStringList disabledInstruments READ getDisabledInstruments WRITE setDisabledInstruments)
  Q_PROPERTY(QString facility READ getFacility WRITE setFacility)
  Q_PROPERTY(bool enableDiffraction READ isDiffractionEnabled WRITE enableDiffraction)
  Q_PROPERTY(bool forceDiffraction READ isDiffractionForced WRITE forceDiffraction)
  Q_PROPERTY(bool showInstrumentLabel READ isInstrumentLabelShown WRITE setShowInstrumentLabel)

public:
  InstrumentConfig(QWidget *parent = nullptr);
  ~InstrumentConfig() override;

  /* Getters and setters for Qt properties */
  QStringList getTechniques() override;
  void setTechniques(const QStringList &techniques) override;

  QStringList getDisabledInstruments() override;
  void setDisabledInstruments(const QStringList &instrumentNames) override;

  QString getFacility() override;
  void setFacility(const QString &facilityName) override;

  bool isDiffractionEnabled() override;
  void enableDiffraction(bool enabled) override;

  bool isDiffractionForced() override;
  void forceDiffraction(bool forced) override;

  bool isInstrumentLabelShown() override;
  void setShowInstrumentLabel(bool visible) override;

  /// Gets the name of the selected instrument
  QString getInstrumentName() override;
  /// Set the displayed instrument (if exists)
  void setInstrument(const QString &instrumentName) override;

  /// Gets the name of the selected analyser
  QString getAnalyserName() override;
  /// Set the displayed analyser bank (if exists)
  void setAnalyser(const QString &analyserName) override;

  /// Gets the name of the selected reflection
  QString getReflectionName() override;
  /// Set the displayed reflection mode (if exists)
  void setReflection(const QString &reflectionName) override;

  /// Controls where to show analyser and reflection options or not
  void showAnalyserAndReflectionOptions(bool visible) override;

public slots:
  /// Called when an instrument configuration is selected
  void newInstrumentConfiguration();
  /// Handles an instrument being selected
  void updateInstrumentConfigurations(const QString &instrumentName);

signals:
  /// Emitted when the instrument configuration is changed
  void instrumentConfigurationUpdated(const QString &instrumentName, const QString &analyserName,
                                      const QString &reflectionName);

private slots:
  /// Updates the list of analysers when an instrument is selected
  bool updateAnalysersList(const Mantid::API::MatrixWorkspace_sptr &ws);
  /// Updates the list of reflections when an analyser is selected
  void updateReflectionsList(int index);
  /// Filters out any disabled instruments
  void filterDisabledInstruments();

private:
  /// Member containing the widgets child widgets.
  Ui::InstrumentConfig m_uiForm;
  /// Instrument selector widget
  MantidQt::MantidWidgets::InstrumentSelector *m_instrumentSelector;

  QStringList m_disabledInstruments;
  bool m_removeDiffraction;
  bool m_forceDiffraction;
};

} /* namespace MantidWidgets */
} /* namespace MantidQt */
