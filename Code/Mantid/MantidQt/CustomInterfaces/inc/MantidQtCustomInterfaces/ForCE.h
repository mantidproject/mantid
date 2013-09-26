#ifndef MANTIDQTCUSTOMINTERFACES_FORCE_H_
#define MANTIDQTCUSTOMINTERFACES_FORCE_H_

#include "ui_ForCE.h"
#include "MantidQtCustomInterfaces/IndirectForeignTab.h"

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
			QString help() { return "ForCE"; };
			bool validate();
			void run();
			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

		private:
			//The ui form
			Ui::ForCE m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
