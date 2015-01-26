#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTTRANSMISSIONCALC_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTTRANSMISSIONCALC_H_

#include "ui_IndirectTransmissionCalc.h"
#include "IndirectToolsTab.h"
#include "MantidAPI/ExperimentInfo.h"

#include <QComboBox>
#include <QMap>
#include <QStringList>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport IndirectTransmissionCalc : public IndirectToolsTab
		{
			Q_OBJECT

		public:
			IndirectTransmissionCalc(QWidget * parent = 0);

			// Inherited methods from IndirectToolsTab
			QString help() { return "Transmission"; };

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

    protected:
      void setup();
			bool validate();
			void run();

    private slots:
      /// Handles completion of the algorithm
      void algorithmComplete(bool error);

		private:
			/// The UI form
			Ui::IndirectTransmissionCalc m_uiForm;
      /// The name of the current instrument
      QString m_instrument;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
