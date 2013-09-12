#ifndef MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_
#define MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_

#include "ui_JumpFit.h"
#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport JumpFit : public IndirectBayesTab
		{
			Q_OBJECT

		public:
			JumpFit(QWidget * parent = 0);

		private:
			virtual QString help() { return "JumpFit"; };
			virtual bool validate();
			virtual void run();

			//The ui form
			Ui::JumpFit m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
