#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTLOADILL_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTLOADILL_H_

#include "ui_IndirectLoadILL.h"
#include "IndirectToolsTab.h"
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

			// Inherited methods from IndirectToolsTab
			QString help() { return "LoadILL"; };

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

    protected:
      void setup();
			bool validate();
			void run();

		private slots:
			/// Set the instrument based on the file name if possible
			void handleFilesFound();

		private:
			/// Map to store instrument analysers and reflections for this instrument
			QMap<QString, QStringList> m_paramMap;
			/// The ui form
			Ui::IndirectLoadILL m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
