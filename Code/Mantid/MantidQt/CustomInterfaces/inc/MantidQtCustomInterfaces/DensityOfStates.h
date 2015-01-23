#ifndef MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_
#define MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_

#include "ui_DensityOfStates.h"
#include "MantidQtCustomInterfaces/IndirectSimulationTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport DensityOfStates : public IndirectSimulationTab
		{
			Q_OBJECT

		public:
			DensityOfStates(QWidget * parent = 0);

			QString help() { return "DensityOfStates"; };

      void setup();
			bool validate();
			void run();

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

		private:
			/// The ui form
			Ui::DensityOfStates m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif //MANTIDQTCUSTOMINTERFACES_DENSITYOFSTATES_H_
