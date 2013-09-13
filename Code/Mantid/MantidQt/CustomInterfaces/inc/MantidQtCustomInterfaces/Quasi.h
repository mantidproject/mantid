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

		private slots:
			/// Slot for when the min range on the range selector changes
			virtual void minValueChanged(double min);
			/// Slot for when the min range on the range selector changes
			virtual void maxValueChanged(double max);
			void updateProperties(QtProperty* prop, double val);

		private:
			virtual QString help() { return "Quasi"; };
			virtual bool validate();
			virtual void run();
			
			//The ui form
			Ui::Quasi m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
