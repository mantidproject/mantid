#ifndef MANTIDQTCUSTOMINTERFACES_FORCE_H_
#define MANTIDQTCUSTOMINTERFACES_FORCE_H_

#include "ui_ForCE.h"
#include "MantidQtCustomInterfaces/IndirectForeignTab.h"
#include "MantidAPI/ExperimentInfo.h"

#include <QComboBox>
#include <QMap>
#include <QStringList>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport ForCE : public IndirectForeignTab
		{
			Q_OBJECT

		public:
			ForCE(QWidget * parent = 0);

			// Inherited methods from IndirectForeignTab
			QString help() { return "Neutron_Force"; };
			bool validate();
			void run();
			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

		private slots:
			/// Populate the analyser and reflection options on the interface
			void instrumentChanged(const QString& instrument);
			/// Populate the reflection option given the analyser
			void analyserChanged(const QString& analyser);
			/// Set the instrument based on the file name if possible
			void handleFilesFound();

		private:
			/// Load the IDF file and get the instrument
			Mantid::Geometry::Instrument_const_sptr getInstrument(const QString& instrument);
			/// Map to store instrument analysers and reflections for this instrument
			QMap<QString, QStringList> m_paramMap;
			/// The ui form
			Ui::ForCE m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
