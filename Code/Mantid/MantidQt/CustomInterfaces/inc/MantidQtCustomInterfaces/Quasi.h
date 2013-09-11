#ifndef MANTIDQTCUSTOMINTERFACES_QUASI_H_
#define MANTIDQTCUSTOMINTERFACES_QUASI_H_

#include "ui_Quasi.h"
#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport Quasi : public IndirectBayesTab
		{
			Q_OBJECT

		public:
			Quasi(QWidget * parent = 0);

		private:
			virtual void help();
			virtual void validate();
			virtual void run();

			//The ui form
			Ui::Quasi m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
