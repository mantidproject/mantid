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

			/// Plot of the input
			QwtPlot* m_plot;
			/// Tree of the properties
			QtTreePropertyBrowser* m_propTree;
			/// Internal list of the properties
			QMap<QString, QtProperty*> m_properties;
			/// Double manager to create properties
			QtDoublePropertyManager* m_dblManager;
			//The ui form
			Ui::Quasi m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
