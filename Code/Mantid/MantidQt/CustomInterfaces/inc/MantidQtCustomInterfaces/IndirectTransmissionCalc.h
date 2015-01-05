#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTTRANSMISSIONCALC_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTTRANSMISSIONCALC_H_

#include "ui_IndirectTransmissionCalc.h"
#include "MantidQtCustomInterfaces/IndirectToolsTab.h"
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

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

    protected:
      void setup();
			bool validate();
			void run();

    private slots:
      /// Handles completion of the algorithm
      void algorithmComplete(bool error);
      /// Handles completion of the instrument loading algorithm
      void instrumentLoadingDone(bool error);
      /// Handles an instrument being selected
      void instrumentSelected(const QString& instrumentName);
      /// Handles an analyser being selected
      void analyserSelected(int);
      /// Enables or disables the instrument selection controls
      void enableInstrumentControls(bool enabled);

		private:
			/// The UI form
			Ui::IndirectTransmissionCalc m_uiForm;
      /// The name of the current instrument
      QString m_instrument;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
