#ifndef MANTIDQTCUSTOMINTERFACES_MOLDYN_H_
#define MANTIDQTCUSTOMINTERFACES_MOLDYN_H_

#include "ui_MolDyn.h"
#include "MantidQtCustomInterfaces/IndirectForeignTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport MolDyn : public IndirectForeignTab
		{
			Q_OBJECT

		public:
			MolDyn(QWidget * parent = 0);

			// Inherited methods from IndirectForeignTab
			QString help() { return "MolDyn"; };
			bool validate();
			void run();
			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

		private:
			//The ui form
			Ui::MolDyn m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
