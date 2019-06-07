// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_INDIRECTINSTRUMENTCONFIG_H_
#define MANTIDQTMANTIDWIDGETS_INDIRECTINSTRUMENTCONFIG_H_

#include "DllConfig.h"
#include "ui_IndirectInstrumentConfig.h"

#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/InstrumentSelector.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {
/**
Widget used to select an instrument configuration for indirect geometry
spectrometers
(i.e. and instrument, analyser and reflection).

Instruments are populated using an InstrumentSelector widget, analysers and
reflections
are populated by loading an empty instrument.

@author Dan Nixon
*/

class MANTIDQT_INDIRECT_DLL IndirectInstrumentConfig
    : public API::MantidWidget {
  Q_OBJECT

  Q_PROPERTY(QStringList techniques READ getTechniques WRITE setTechniques)
  Q_PROPERTY(QStringList disabledInstruments READ getDisabledInstruments WRITE
                 setDisabledInstruments)
  Q_PROPERTY(QString facility READ getFacility WRITE setFacility)
  Q_PROPERTY(
      bool enableDiffraction READ isDiffractionEnabled WRITE enableDiffraction)
  Q_PROPERTY(
      bool forceDiffraction READ isDiffractionForced WRITE forceDiffraction)
  Q_PROPERTY(bool showInstrumentLabel READ isInstrumentLabelShown WRITE
                 setShowInstrumentLabel)

public:
  IndirectInstrumentConfig(QWidget *parent = nullptr);
  ~IndirectInstrumentConfig() override;

  /* Getters and setters for Qt properties */
  QStringList getTechniques();
  void setTechniques(const QStringList &techniques);

  QStringList getDisabledInstruments();
  void setDisabledInstruments(const QStringList &instrumentNames);

  QString getFacility();
  void setFacility(const QString &facilityName);

  bool isDiffractionEnabled();
  void enableDiffraction(bool enabled);

  bool isDiffractionForced();
  void forceDiffraction(bool forced);

  bool isInstrumentLabelShown();
  void setShowInstrumentLabel(bool visible);

  /// Gets the name of the selected instrument
  QString getInstrumentName();
  /// Set the displayed instrument (if exists)
  void setInstrument(const QString &instrumentName);

  /// Gets the name of the selected analyser
  QString getAnalyserName();
  /// Set the displayed analyser bank (if exists)
  void setAnalyser(const QString &analyserName);

  /// Gets the name of the selected reflection
  QString getReflectionName();
  /// Set the displayed reflection mode (if exists)
  void setReflection(const QString &reflectionName);

public slots:
  /// Called when an instrument configuration is selected
  void newInstrumentConfiguration();

signals:
  /// Emitted when the instrument configuration is changed
  void instrumentConfigurationUpdated(const QString &instrumentName,
                                      const QString &analyserName,
                                      const QString &reflectionName);

private slots:
  /// Handles an instrument being selected
  void updateInstrumentConfigurations(const QString &instrumentName);
  /// Updates the list of analysers when an instrument is selected
  bool updateAnalysersList(Mantid::API::MatrixWorkspace_sptr ws);
  /// Updates the list of reflections when an analyser is selected
  void updateReflectionsList(int index);
  /// Filters out any disabled instruments
  void filterDisabledInstruments();

private:
  /// Member containing the widgets child widgets.
  Ui::IndirectInstrumentConfig m_uiForm;
  /// Algorithm Runner used to load empty instrument workspaces
  MantidQt::API::AlgorithmRunner m_algRunner;
  /// Instrument selector widget
  MantidQt::MantidWidgets::InstrumentSelector *m_instrumentSelector;

  QStringList m_disabledInstruments;
  bool m_removeDiffraction;
  bool m_forceDiffraction;
};

} /* namespace MantidWidgets */
} /* namespace MantidQt */

#endif /* INDIRECTINSTRUMENTCONFIG_H_ */
