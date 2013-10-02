#ifndef MANTIDQTCUSTOMINTERFACES_IndirectMolDyn_H_
#define MANTIDQTCUSTOMINTERFACES_IndirectMolDyn_H_

#include "ui_IndirectMolDyn.h"
#include "MantidQtCustomInterfaces/IndirectLoadAsciiTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport IndirectMolDyn : public IndirectLoadAsciiTab
		{
			Q_OBJECT

		public:
			IndirectMolDyn(QWidget * parent = 0);

			// Inherited methods from IndirectForeignTab
			QString help() { return "IndirectMolDyn"; };
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
