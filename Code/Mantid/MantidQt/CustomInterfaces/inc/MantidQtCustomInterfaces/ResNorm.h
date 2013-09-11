#ifndef MANTIDQTCUSTOMINTERFACES_RESNORM_H_
#define MANTIDQTCUSTOMINTERFACES_RESNORM_H_

#include "ui_ResNorm.h"
#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport ResNorm : public IndirectBayesTab
		{
			Q_OBJECT

		public:
			ResNorm(QWidget * parent = 0);

		private:
			virtual void help();
			virtual void validate();
			virtual void run();
	
			//The ui form
			Ui::ResNorm m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
