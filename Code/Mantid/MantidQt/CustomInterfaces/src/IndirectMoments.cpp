#include "MantidQtCustomInterfaces/IndirectMoments.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QDoubleValidator>
#include <QFileInfo>

namespace MantidQt
{
namespace CustomInterfaces
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectMoments::IndirectMoments(Ui::IndirectDataReduction& uiForm, QWidget * parent) : IndirectDataReductionTab(uiForm, parent)
  {
    const unsigned int NUM_DECIMALS = 6;

    m_plots["MomentsPlot"] = new QwtPlot(m_parentWidget);
    m_curves["MomentsPlotCurve"] = new QwtPlotCurve();
    m_rangeSelectors["MomentsRangeSelector"] = new MantidWidgets::RangeSelector(m_plots["MomentsPlot"]);
    m_propTrees["MomentsPropTree"] = new QtTreePropertyBrowser();

    m_propTrees["MomentsPropTree"]->setFactoryForManager(m_dblManager, m_dblEdFac);
    m_rangeSelectors["MomentsRangeSelector"]->setInfoOnly(false);

    // initilise plot
    m_plots["MomentsPlot"]->setCanvasBackground(Qt::white);
    m_plots["MomentsPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["MomentsPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());

    //add the plot to the ui form
    m_uiForm.moment_plotSpace->addWidget(m_plots["MomentsPlot"]);
    //add the properties browser to the ui form
    m_uiForm.moment_treeSpace->addWidget(m_propTrees["MomentsPropTree"]);
    m_properties["EMin"] = m_dblManager->addProperty("EMin");
    m_properties["EMax"] = m_dblManager->addProperty("EMax");

    m_propTrees["MomentsPropTree"]->addProperty(m_properties["EMin"]);
    m_propTrees["MomentsPropTree"]->addProperty(m_properties["EMax"]);

    m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
    
    m_uiForm.moment_leScale->setValidator(new QDoubleValidator());

    connect(m_uiForm.moment_dsInput, SIGNAL(dataReady(const QString&)), this, SLOT(handleSampleInputReady(const QString&)));
    connect(m_uiForm.moment_ckScale, SIGNAL(toggled(bool)), m_uiForm.moment_leScale, SLOT(setEnabled(bool)));
    connect(m_uiForm.moment_ckScale, SIGNAL(toggled(bool)), m_uiForm.moment_validScale, SLOT(setVisible(bool)));
    
    connect(m_rangeSelectors["MomentsRangeSelector"], SIGNAL(minValueChanged(double)), this, SLOT(minValueChanged(double)));
    connect(m_rangeSelectors["MomentsRangeSelector"], SIGNAL(maxValueChanged(double)), this, SLOT(maxValueChanged(double)));
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateProperties(QtProperty*, double)));

    m_uiForm.moment_validScale->setStyleSheet("QLabel { color : #aa0000; }");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectMoments::~IndirectMoments()
  {
  }
  
  void IndirectMoments::setup()
  {
  }

  void IndirectMoments::run()
  {
    using namespace Mantid::API;
    QString workspaceName = m_uiForm.moment_dsInput->getCurrentDataName();
    QString outputName = workspaceName.left(workspaceName.length()-4);
    QString scaleString = m_uiForm.moment_leScale->text();
    double scale = 1.0;
    double eMin = m_dblManager->value(m_properties["EMin"]);
    double eMax = m_dblManager->value(m_properties["EMax"]);

    bool plot = m_uiForm.moment_ckPlot->isChecked();
    bool verbose = m_uiForm.moment_ckVerbose->isChecked();
    bool save = m_uiForm.moment_ckSave->isChecked();

    if (!scaleString.isEmpty())
    {
      scale = scaleString.toDouble();
    }

    IAlgorithm_sptr momentsAlg = AlgorithmManager::Instance().create("SofQWMoments", -1);
    momentsAlg->initialize();
    momentsAlg->setProperty("Sample", workspaceName.toStdString());
    momentsAlg->setProperty("Scale", scale);
    momentsAlg->setProperty("EnergyMin", eMin);
    momentsAlg->setProperty("EnergyMax", eMax);
    momentsAlg->setProperty("Plot", plot);
    momentsAlg->setProperty("Verbose", verbose);
    momentsAlg->setProperty("Save", save);
    momentsAlg->setProperty("OutputWorkspace", outputName.toStdString() + "_Moments");

    //execute algorithm on seperate thread
    runAlgorithm(momentsAlg);
  }

  bool IndirectMoments::validate()
  {
    using namespace Mantid::API;
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample input", m_uiForm.moment_dsInput);

    if (m_uiForm.moment_ckScale->isChecked())
    {
      uiv.checkFieldIsValid("A valid scale must be supplied.\n", m_uiForm.moment_leScale, m_uiForm.moment_validScale);
    }

    QString msg = uiv.generateErrorMessage();
    if (!msg.isEmpty())
    {
      emit showMessageBox(msg);
      return false;
    } 

    return true;
  }

  void IndirectMoments::handleSampleInputReady(const QString& filename)
  {
    plotMiniPlot(filename, 0, "MomentsPlot", "MomentsPlotCurve");
    std::pair<double,double> range = getCurveRange("MomentsPlotCurve");
    setMiniPlotGuides("MomentsRangeSelector", m_properties["EMin"], m_properties["EMax"], range);
    setPlotRange("MomentsRangeSelector", m_properties["EMin"], m_properties["EMax"], range);
  }

  /**
   * Updates the property manager when the lower guide is moved on the mini plot
   *
   * @param min :: The new value of the lower guide
   */
  void IndirectMoments::minValueChanged(double min)
  {
    m_dblManager->setValue(m_properties["EMin"], min);
  }

  /**
   * Updates the property manager when the upper guide is moved on the mini plot
   *
   * @param max :: The new value of the upper guide
   */
  void IndirectMoments::maxValueChanged(double max)
  {
    m_dblManager->setValue(m_properties["EMax"], max);  
  }

   /**
   * Handles when properties in the property manager are updated.
   *
   * @param prop :: The property being updated
   * @param val :: The new value for the property
   */
  void IndirectMoments::updateProperties(QtProperty* prop, double val)
  {
    if(prop == m_properties["EMin"])
    {
      double emax = m_dblManager->value(m_properties["EMax"]);
      if(val >  emax) 
      {
        m_dblManager->setValue(prop, emax);
      }
      else 
      {
        m_rangeSelectors["MomentsRangeSelector"]->setMinimum(val);
      }
    }
    else if (prop == m_properties["EMax"])
    {
      double emin = m_dblManager->value(m_properties["EMin"]);
      if(emin > val) 
      {
        m_dblManager->setValue(prop, emin);
      }
      else 
      {
        m_rangeSelectors["MomentsRangeSelector"]->setMaximum(val);
      }
    }
  }

} // namespace CustomInterfaces
} // namespace Mantid
