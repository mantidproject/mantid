#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_

#include "ui_IndirectSassena.h"
#include "IndirectSimulationTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport IndirectSassena : public IndirectSimulationTab
		{
			Q_OBJECT

		public:
			IndirectSassena(QWidget * parent = 0);

			QString help() { return "IndirectSassena"; };

      void setup();
			bool validate();
			void run();

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

    private slots:
      /// Handle completion of the algorithm batch
      void handleAlgorithmFinish(bool error);

		private:
			/// The ui form
			Ui::IndirectSassena m_uiForm;
      /// Name of the output workspace group
      QString m_outWsName;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_
