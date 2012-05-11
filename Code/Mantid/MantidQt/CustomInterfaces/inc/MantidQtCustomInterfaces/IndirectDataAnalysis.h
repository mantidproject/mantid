#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectDataAnalysis.h"
#include "MantidQtAPI/UserSubWindow.h"

#include <Poco/NObserver.h>
#include "MantidKernel/ConfigService.h"

//----------------------
// Forward Declarations
//----------------------
namespace Mantid
{
  namespace API
  {
    class MatrixWorkspace;
    class IFunction;
    class CompositeFunction;
  }
}
namespace MantidQt
{
  namespace MantidWidgets
  {
    class RangeSelector;
  }
}

class QwtPlot;
class QwtPlotCurve;
class QIntValidator;
class QDoubleValidator;
class DoubleEditorFactory;
class QtCheckBoxFactory;
class QtProperty;
class QtBoolPropertyManager;
class QtDoublePropertyManager;
class QtGroupPropertyManager;
class QtStringPropertyManager;
class QtTreePropertyBrowser;
//----------------------
//       End Of
// Forward Declarations
//----------------------

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  // The assumption is made elsewhere that the ordering of these enums matches the ordering of the
  // tabs as they appear in the interface itself.
  enum TabChoice
  {
    ELWIN,
    MSD_FIT,
    FURY,
    FURY_FIT,
    CON_FIT,
    ABSORPTION_F2PY,
    ABS_COR
  };
    
  // Forward Declaration
  class IDATab;
    
  /**
   * The IndirectDataAnalysis class is the main class that handles the interface and controls
   * its tabs.
   *
   * Is a friend to the IDATab class.
   */
  class IndirectDataAnalysis : public MantidQt::API::UserSubWindow
  {
    Q_OBJECT

    friend class IDATab;

  public:
    /// The name of the interface as registered into the factory
    static std::string name() { return "Indirect Data Analysis"; }
    /// Default Constructor
    IndirectDataAnalysis(QWidget *parent = 0);

  private:
    /// Initialize the layout
    virtual void initLayout();
    /// Initialize Python-dependent sections
    virtual void initLocalPython();


    void loadSettings();

    QwtPlotCurve* plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, std::string workspace, size_t index);
    std::pair<double,double> getCurveRange(QwtPlotCurve* curve);
      
    virtual void closeEvent(QCloseEvent*);

    void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf); ///< handle POCO event

  private slots:
    void run();
      
    // Common Elements
    void openDirectoryDialog();
    void help();
      
  private:
    Ui::IndirectDataAnalysis m_uiForm;
    int NUM_DECIMALS;
    QIntValidator* m_valInt;
    QDoubleValidator* m_valDbl;
            
    QtStringPropertyManager* m_stringManager;
    // Editor Factories
    DoubleEditorFactory* m_dblEdFac;
    QtCheckBoxFactory* m_blnEdFac;

    /// Change Observer for ConfigService (monitors user directories)
    Poco::NObserver<IndirectDataAnalysis, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;

    std::map<unsigned int, IDATab*> m_tabs;
  };



  class IDATab : public QWidget
  {
    Q_OBJECT

  public:
    static const unsigned int NUM_DECIMALS;

  public:
    /// Constructor
    IDATab(QWidget * parent = 0) : QWidget(parent), m_parent(NULL)
    {
      m_parent = dynamic_cast<IndirectDataAnalysis*>(parent);
    }

    /// Calls overidden setupTab function of child classes.  NVI idiom.
    void setupTab() { setup(); }

    /// Calls overidden runTab function of child classes.  NVI idiom.
    void runTab()
    { 
      const QString error = validate();

      if( ! error.isEmpty() )
        showInformationBox(error);
      else
        run();
    }

    void loadTabSettings(const QSettings & settings)
    {
      loadSettings(settings);
    }

  protected:
    void showInformationBox(const QString & message);

    QwtPlotCurve* plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, const std::string & workspace, size_t index);
    std::pair<double,double> getCurveRange(QwtPlotCurve* curve);

    /// Run a piece of python code and return any output that was written to stdout
    QString runPythonCode(const QString & code, bool no_output = false);

    Ui::IndirectDataAnalysis & uiForm();
    DoubleEditorFactory * doubleEditorFactory();
    QtCheckBoxFactory * qtCheckBoxFactory();
      
  protected slots:
    // context menu on fitting property browser
    void fitContextMenu(const QPoint &);
    void fixItem();
    void unFixItem();

  private:
    /// Overidden by child class.
    virtual void setup() = 0;
    /// Overidden by child class.
    virtual void run() = 0;
    /// Overidden by child class.
    virtual QString validate() = 0;
    /// Overidden by child class.
    virtual void loadSettings(const QSettings & settings) = 0;

    IndirectDataAnalysis * m_parent;
  };
    
  class Elwin : public IDATab
  {
    Q_OBJECT

  public:
    Elwin(QWidget * parent = 0) : IDATab(parent) {}

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);

  private slots:
    void plotInput();
    void twoRanges(QtProperty *, bool);
    void minChanged(double val);
    void maxChanged(double val);
    void updateRS(QtProperty * prop, double val);

  private:
    QwtPlot* m_elwPlot;
    MantidWidgets::RangeSelector* m_elwR1;
    MantidWidgets::RangeSelector* m_elwR2;
    QwtPlotCurve* m_elwDataCurve;
    QtTreePropertyBrowser* m_elwTree;
    QMap<QString, QtProperty*> m_elwProp;
    QtDoublePropertyManager* m_elwDblMng;
    QtBoolPropertyManager* m_elwBlnMng;
    QtGroupPropertyManager* m_elwGrpMng;
  };
    
  class MSDFit : public IDATab
  {
    Q_OBJECT

  public:
    MSDFit(QWidget * parent = 0) : IDATab(parent) {}

  private slots:
    void plotInput();
    void minChanged(double val);
    void maxChanged(double val);
    void updateRS(QtProperty* prop, double val);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
      
    QwtPlot* m_msdPlot;
    MantidWidgets::RangeSelector* m_msdRange;
    QwtPlotCurve* m_msdDataCurve;
    QtTreePropertyBrowser* m_msdTree;
    QMap<QString, QtProperty*> m_msdProp;
    QtDoublePropertyManager* m_msdDblMng;
  };
    
  class Fury : public IDATab
  {
    Q_OBJECT

  public:
    Fury(QWidget * parent = 0) : IDATab(parent) {}

  private slots:
    void resType(const QString& type);
    void plotInput();
    void minChanged(double val);
    void maxChanged(double val);
    void updateRS(QtProperty* prop, double val);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
      
    QwtPlot* m_furPlot;
    MantidWidgets::RangeSelector* m_furRange;
    QwtPlotCurve* m_furCurve;
    QtTreePropertyBrowser* m_furTree;
    QMap<QString, QtProperty*> m_furProp;
    QtDoublePropertyManager* m_furDblMng;
    bool m_furyResFileType;
  };
    
  class FuryFit : public IDATab
  {
    Q_OBJECT

  public:
    FuryFit(QWidget * parent = 0) : 
      IDATab(parent), m_intVal(NULL) 
    {
      m_intVal = new QIntValidator(this);
    }

  private slots:
    void typeSelection(int index);
    void plotInput();
    void xMinSelected(double val);
    void xMaxSelected(double val);
    void backgroundSelected(double val);
    void rangePropChanged(QtProperty*, double);
    void sequential();
    void plotGuess(QtProperty*);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);

    boost::shared_ptr<Mantid::API::CompositeFunction> createFunction(bool tie=false);
    boost::shared_ptr<Mantid::API::IFunction> createUserFunction(const QString & name, bool tie=false);
    QtProperty* createExponential(const QString &);
    QtProperty* createStretchedExp(const QString &);
      
    QIntValidator * m_intVal;

    QtTreePropertyBrowser* m_ffTree; ///< FuryFit Property Browser
    QtGroupPropertyManager* m_groupManager;
    QtDoublePropertyManager* m_ffDblMng;
    QtDoublePropertyManager* m_ffRangeManager; ///< StartX and EndX for FuryFit
    QMap<QString, QtProperty*> m_ffProp;
    QwtPlot* m_ffPlot;
    QwtPlotCurve* m_ffDataCurve;
    QwtPlotCurve* m_ffFitCurve;
    MantidQt::MantidWidgets::RangeSelector* m_ffRangeS;
    MantidQt::MantidWidgets::RangeSelector* m_ffBackRangeS;
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_ffInputWS;
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_ffOutputWS;
    std::string m_ffInputWSName;
    QString m_furyfitTies;
  };
    
  class ConFit : public IDATab
  {
    Q_OBJECT

  public:
    ConFit(QWidget * parent = 0) : 
      IDATab(parent), m_intVal(NULL) 
    {
      m_intVal = new QIntValidator(this);
    }

  private slots:
    void typeSelection(int index);
    void bgTypeSelection(int index);
    void plotInput();
    void plotGuess(QtProperty*);
    void sequential();
    void minChanged(double);
    void maxChanged(double);
    void backgLevel(double);
    void updateRS(QtProperty*, double);
    void checkBoxUpdate(QtProperty*, bool);
    void hwhmChanged(double);
    void hwhmUpdateRS(double);

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);

    boost::shared_ptr<Mantid::API::CompositeFunction> createFunction(bool tie=false);
    QtProperty* createLorentzian(const QString &);
    void populateFunction(boost::shared_ptr<Mantid::API::IFunction>, boost::shared_ptr<Mantid::API::IFunction>, QtProperty*, const std::string & pref, const bool tie=false);
      
    QIntValidator * m_intVal;
      
    QtTreePropertyBrowser* m_cfTree;
    QwtPlot* m_cfPlot;
    QMap<QString, QtProperty*> m_cfProp;
    QMap<QtProperty*, QtProperty*> m_fixedProps;
    MantidWidgets::RangeSelector* m_cfRangeS;
    MantidWidgets::RangeSelector* m_cfBackgS;
    MantidWidgets::RangeSelector* m_cfHwhmRange;
    QtGroupPropertyManager* m_cfGrpMng;
    QtDoublePropertyManager* m_cfDblMng;
    QtBoolPropertyManager* m_cfBlnMng;
    QwtPlotCurve* m_cfDataCurve;
    QwtPlotCurve* m_cfCalcCurve;
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_cfInputWS;
    std::string m_cfInputWSName;
  };
    
  class AbsorptionF2Py : public IDATab
  {
    Q_OBJECT

  public:
    AbsorptionF2Py(QWidget * parent = 0) : 
      IDATab(parent), m_dblVal(NULL) 
    {
      m_dblVal = new QDoubleValidator(this);
    }

  private slots:
    void shape(int index);
    void useCanChecked(bool checked);
    void tcSync();

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
      
    QDoubleValidator * m_dblVal;
  };
    
  class AbsCor : public IDATab
  {
    Q_OBJECT

  public:
    AbsCor(QWidget * parent = 0) : IDATab(parent) {}

  private:
    virtual void setup();
    virtual void run();
    virtual QString validate();
    virtual void loadSettings(const QSettings & settings);
  };

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_ */
