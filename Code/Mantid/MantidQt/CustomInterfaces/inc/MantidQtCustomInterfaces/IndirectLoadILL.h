#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTLOADILL_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTLOADILL_H_

#include "ui_IndirectLoadILL.h"
#include "MantidQtCustomInterfaces/IndirectToolsTab.h"
#include "MantidAPI/ExperimentInfo.h"

#include <QComboBox>
#include <QMap>
#include <QStringList>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport IndirectLoadILL : public IndirectToolsTab
		{
			Q_OBJECT

		public:
			IndirectLoadILL(QWidget * parent = 0);

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

    protected:
      void setup();
			bool validate();
			void run();

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
			Ui::IndirectLoadILL m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
