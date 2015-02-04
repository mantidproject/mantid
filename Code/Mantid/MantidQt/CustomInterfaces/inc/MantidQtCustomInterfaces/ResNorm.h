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

			// Inherited methods from IndirectBayesTab
      void setup();
			bool validate();
			void run();
			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

		private slots:
			/// Handle when the vanadium input is ready
			void handleVanadiumInputReady(const QString& filename);
			/// Slot for when the min range on the range selector changes
			void minValueChanged(double min);
			/// Slot for when the min range on the range selector changes
			void maxValueChanged(double max);
			/// Slot to update the guides when the range properties change
			void updateProperties(QtProperty* prop, double val);
      /// Slot to handle the preview spectrum being changed
      void previewSpecChanged(int value);

		private:
      /// Current preview spectrum
      int m_previewSpec;
			/// The ui form
			Ui::ResNorm m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
