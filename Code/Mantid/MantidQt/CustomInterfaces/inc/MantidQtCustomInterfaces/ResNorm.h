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

		private slots:
			/// Handle when the vanadium input is ready
			void handleVanadiumInputReady(const QString& filename);
			/// Slot for when the min range on the range selector changes
			virtual void minValueChanged(double min);
			/// Slot for when the min range on the range selector changes
			virtual void maxValueChanged(double max);
			/// Slot to update the guides when the range properties change
			void updateProperties(QtProperty* prop, double val);

		private:
			/// Inherited methods from IndirectBayesTab
			virtual QString help() { return "ResNorm"; };
			virtual bool validate();
			virtual void run();
			
			//The ui form
			Ui::ResNorm m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
