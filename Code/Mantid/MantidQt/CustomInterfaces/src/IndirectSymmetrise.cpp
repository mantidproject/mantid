#include "MantidQtCustomInterfaces/IndirectSymmetrise.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>

namespace
{
  Mantid::Kernel::Logger g_log("IndirectSymmetrise");
}

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectSymmetrise::IndirectSymmetrise(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
    // Property Tree
    m_propTrees["SymmPropTree"] = new QtTreePropertyBrowser();
    m_uiForm.symm_properties->addWidget(m_propTrees["SymmPropTree"]);

    // Editor Factories
    DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
    m_propTrees["SymmPropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);

    // Create Properties
    m_properties["XCut"] = m_dblManager->addProperty("X Cut");
    /* m_dblManager->setMinimum(m_properties["XCut"], 0); */
    m_propTrees["SymmPropTree"]->addProperty(m_properties["XCut"]);

    // Raw plot
    m_plots["SymmRawPlot"] = new QwtPlot(m_parentWidget);
    m_curves["SymmRawPlot"] = new QwtPlotCurve();

    m_rangeSelectors["SlicePeak"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"]);
    m_rangeSelectors["SliceBackground"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"]);

    m_plots["SymmRawPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["SymmRawPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["SymmRawPlot"]->setCanvasBackground(Qt::white);
    m_uiForm.symm_plot->addWidget(m_plots["SymmRawPlot"]);

    // Refresh the plot windows
    /* m_plots["SymmRawPlot"]->replot(); */

    // SIGNAL/SLOT CONNECTIONS

    /* // Update properties when a range selector is changed */
    /* connect(m_rangeSelectors["SlicePeak"], SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double))); */
    /* connect(m_rangeSelectors["SlicePeak"], SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double))); */
    /* connect(m_rangeSelectors["SliceBackground"], SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double))); */
    /* connect(m_rangeSelectors["SliceBackground"], SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double))); */
    /* // Update range selctors when a property is changed */
    /* connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(sliceUpdateRS(QtProperty*, double))); */
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectSymmetrise::~IndirectSymmetrise()
  {
  }
  
  void IndirectSymmetrise::setup()
  {
  }

  void IndirectSymmetrise::run()
  {
    //TODO
  }

  bool IndirectSymmetrise::validate()
  {
    //TODO
    return false;
  }

  void IndirectSymmetrise::plotRawInput()
  {
    //TODO
  }

  void IndirectSymmetrise::updateRangeSelector(QtProperty *prop, double value)
  {
    //TODO
  }

} // namespace CustomInterfaces
} // namespace Mantid
