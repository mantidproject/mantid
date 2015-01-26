#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTMOLDYN_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTMOLDYN_H_

#include "ui_IndirectMolDyn.h"
#include "IndirectSimulationTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport IndirectMolDyn : public IndirectSimulationTab
		{
			Q_OBJECT

		public:
			IndirectMolDyn(QWidget * parent = 0);

			QString help() { return "IndirectMolDyn"; };

			// Inherited methods from IndirectTab
      void setup();
			bool validate();
			void run();

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

		private:
			//The ui form
			Ui::IndirectMolDyn m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
