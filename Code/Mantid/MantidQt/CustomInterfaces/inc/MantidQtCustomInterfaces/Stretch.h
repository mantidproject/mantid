#ifndef MANTIDQTCUSTOMINTERFACES_STRETCH_H_
#define MANTIDQTCUSTOMINTERFACES_STRETCH_H_

#include "ui_Stretch.h"
#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport Stretch : public IndirectBayesTab
		{
			Q_OBJECT

		public:
			Stretch(QWidget * parent = 0);

		private slots:
			/// Slot for when the min range on the range selector changes
			virtual void minValueChanged(double min);
			/// Slot for when the min range on the range selector changes
			virtual void maxValueChanged(double max);
			/// Slot to update the guides when the range properties change
			void updateProperties(QtProperty* prop, double val);
			/// Slot to handle when a new sample file is available
			void handleSampleInputReady(const QString& filename);

		private:
			/// Inherited methods from IndirectBayesTab
			virtual QString help() { return "Stretch"; };
			virtual bool validate();
			virtual void run();
			
			//The ui form
			Ui::Stretch m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
