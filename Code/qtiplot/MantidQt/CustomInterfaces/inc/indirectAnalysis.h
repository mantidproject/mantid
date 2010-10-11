#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ui_indirectAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidAPI/CompositeFunction.h"

#include <QIntValidator>
#include <QDoubleValidator>

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class indirectAnalysis : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// The name of the interface as registered into the factory
      static std::string name() { return "Indirect Data Analysis"; }

    public:
      /// Default Constructor
      indirectAnalysis(QWidget *parent = 0);

    protected:
      virtual void closeEvent(QCloseEvent* close);

    private:
      /// Initialize the layout
      virtual void initLayout();
      /// init python-dependent sections
      virtual void initLocalPython();

      void loadSettings();
      void saveSettings();

      void setupTreePropertyBrowser();
      void setupFFPlotArea();

      bool validateFury();
      bool validateElwin();
      bool validateMsd();
      bool validateAbsorption();
      bool validateDemon();

      Mantid::API::CompositeFunction* createFunction();

      QtProperty* createLorentzian();
      QtProperty* createStretchedExp();
      QtProperty* createExponential();

    private slots:

      void instrumentChanged(int index);
      void analyserSelected(int index);
      void reflectionSelected(int index);
      void furyRun();
      void furyResType(const QString& type);
      void furyPlotInput();

      void runFuryFit();
      void furyfit_typeSelection(int index);
      void furyfitPlotInput();
      void furyfitXMinSelected(double val);
      void furyfitXMaxSelected(double val);
      void furyfitRangePropChanged(QtProperty*, double);

      void elwinRun();
      void elwinPlotInput();
      void elwinTwoRanges(bool state);
      void msdRun();
      void msdPlotInput();
      void absorptionRun();
      void absorptionShape(int index);
      void demonRun();

      void openDirectoryDialog();
      void help();


    private:
      Ui::indirectAnalysis m_uiForm;

      QString m_settingsGroup;
      
      QString m_dataDir;
      QString m_saveDir;

      QIntValidator *m_valInt;
      QDoubleValidator *m_valDbl;

      QtTreePropertyBrowser* m_propBrowser;

      QtGroupPropertyManager* m_groupManager;
      QtDoublePropertyManager* m_doubleManager;
      QtDoublePropertyManager* m_ffRangeManager;

      bool m_furyResFileType;
      
      QMap<QString, QtProperty*> m_fitProperties;

      QwtPlot* m_furyFitPlotWindow;
      QwtPlotCurve* m_ffDataCurve;
      MantidQt::MantidWidgets::RangeSelector* m_ffRangeS;

    };
  }
}
#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_