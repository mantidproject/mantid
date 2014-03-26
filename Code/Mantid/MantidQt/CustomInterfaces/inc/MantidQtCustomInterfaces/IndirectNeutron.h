#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTNEUTRON_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTNEUTRON_H_

#include "ui_IndirectNeutron.h"
#include "MantidQtCustomInterfaces/IndirectLoadAsciiTab.h"
#include "MantidAPI/ExperimentInfo.h"

#include <QComboBox>
#include <QMap>
#include <QStringList>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport IndirectNeutron : public IndirectLoadAsciiTab
		{
			Q_OBJECT

		public:
			IndirectNeutron(QWidget * parent = 0);

			// Inherited methods from IndirectLoadAsciiTab
			QString help() { return "Indirect_Neutron"; };
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
			Ui::IndirectNeutron m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
