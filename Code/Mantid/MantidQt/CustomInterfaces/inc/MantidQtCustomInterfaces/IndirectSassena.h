#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_

#include "ui_IndirectSassena.h"
#include "MantidQtCustomInterfaces/IndirectSimulationTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport IndirectSassena : public IndirectSimulationTab
		{
			Q_OBJECT

		public:
			IndirectSassena(QWidget * parent = 0);

			QString help() { return "IndirectSassena"; };

      void setup();
			bool validate();
			void run();

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

		private:
			// The ui form
			Ui::IndirectSassena m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_
