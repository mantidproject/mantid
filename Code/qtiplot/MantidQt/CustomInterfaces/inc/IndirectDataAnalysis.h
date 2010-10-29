#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ui_IndirectDataAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

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
    class IndirectDataAnalysis : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// The name of the interface as registered into the factory
      static std::string name() { return "Indirect Data Analysis"; }
      /// Default Constructor
      IndirectDataAnalysis(QWidget *parent = 0);

    private:
      /// Initialize the layout
      virtual void initLayout();
      /// init python-dependent sections
      virtual void initLocalPython();

      void loadSettings();
      void saveSettings();

      // Tab-specific setup stages (mainly miniplots)
      void setupFuryFit();
      void setupElwin();

      // Validation of user inputs
      bool validateElwin();
      bool validateMsd();
      bool validateFury();
      bool validateAbsorption();
      bool validateDemon();

      // Run processes
      void elwinRun();
      void msdRun();
      void furyRun();
      void furyfitRun();
      void absorptionRun();
      void demonRun();

      Mantid::API::CompositeFunction* createFunction(QtTreePropertyBrowser* propertyBrowser);
      QtProperty* createLorentzian();
      QtProperty* createExponential();
      QtProperty* createStretchedExp();
      
      virtual void closeEvent(QCloseEvent* close);

      void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf); ///< handle POCO event

    private slots:
      // Generic
      void instrumentChanged(int index);
      void analyserSelected(int index);
      void reflectionSelected(int index);
      void refreshWSlist();

      void run();

      // ElasticWindow
      void elwinPlotInput();
      void elwinTwoRanges(bool state);
      void elwinMinChanged(double val);
      void elwinMaxChanged(double val);
      void elwinUpdateRS();

      // MSD Fit
      void msdPlotInput();

      // Fourier Transform
      void furyInputType(int index);
      void furyResType(const QString& type);
      void furyPlotInput();

      // Fourier Transform Fit
      void furyfitTypeSelection(int index);
      void furyfitPlotInput();
      void furyfitXMinSelected(double val);
      void furyfitXMaxSelected(double val);
      void furyfitBackgroundSelected(double val);
      void furyfitRangePropChanged(QtProperty*, double);
      void furyfitInputType(int index);
      void furyfitPlotOutput();
      void furyfitSequential();
      void furyfitPlotGuess(QtProperty* prop);

      // Convolution Fit

      // Absorption (Basic)
      void absorptionShape(int index);

      // Diffraction Reduction

      // Common Elements
      void help();
      void openDirectoryDialog();
      
    private:
      Ui::IndirectDataAnalysis m_uiForm;
      QString m_settingsGroup;
      QString m_dataDir;
      QString m_saveDir;
      QIntValidator *m_valInt;
      QDoubleValidator *m_valDbl;

      bool m_furyResFileType;

      // Fury Fit Member Variables (prefix 'm_ff')
      QtTreePropertyBrowser* m_ffTree; ///< FuryFit Property Browser
      QtGroupPropertyManager* m_groupManager;
      QtDoublePropertyManager* m_doubleManager;
      QtDoublePropertyManager* m_ffRangeManager; ///< StartX and EndX for FuryFit
      QMap<QString, QtProperty*> m_ffProp;
      QwtPlot* m_furyFitPlotWindow;
      QwtPlotCurve* m_ffDataCurve;
      QwtPlotCurve* m_ffFitCurve;
      MantidQt::MantidWidgets::RangeSelector* m_ffRangeS;
      MantidQt::MantidWidgets::RangeSelector* m_ffBackRangeS;
      /// keep a pointer to the input workspace once it's loaded so we don't have to
      Mantid::API::MatrixWorkspace_const_sptr m_ffInputWS; ///< load it again and again etc
      Mantid::API::MatrixWorkspace_const_sptr m_ffOutputWS;
      std::string m_ffInputWSName;
      QString m_furyfitTies;
      QString m_furyfitConstraints;

      // ELASTICWINDOW MINIPLOT (prefix: 'm_elw')
      QwtPlot* m_elwPlot;
      MantidWidgets::RangeSelector* m_elwR1;
      MantidWidgets::RangeSelector* m_elwR2;
      QwtPlotCurve* m_elwDataCurve;

      /// Change Observer for ConfigService (monitors user directories)
      Poco::NObserver<IndirectDataAnalysis, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;

    };
  }
}
#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_