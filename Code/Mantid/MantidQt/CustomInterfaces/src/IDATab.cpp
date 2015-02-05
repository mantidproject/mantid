#include "MantidQtCustomInterfaces/IDATab.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "boost/shared_ptr.hpp"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <QString>

using namespace Mantid::API;
  
namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  /**
   * Constructor.
   *
   * @param parent :: the parent widget (an IndirectDataAnalysis object).
   */
  IDATab::IDATab(QWidget * parent) : IndirectTab(parent),
    m_parent(NULL)
  {
    m_parent = dynamic_cast<IndirectDataAnalysis*>(parent);
  }

  /**
   * Loads the tab's settings.
   *
   * Calls overridden version of loadSettings() in child class.
   *
   * @param settings :: the QSettings object from which to load
   */
  void IDATab::loadTabSettings(const QSettings & settings)
  {
    loadSettings(settings);
  }

  /**
   * Slot that can be called when a user edits an input.
   */
  void IDATab::inputChanged()
  {
    validate();
  }

  /**
  * Check that the binning between two workspaces matches.
  *
  * @param left :: left hand workspace for the equality operator
  * @param right :: right hand workspace for the equality operator
  * @return whether the binning matches
  * @throws std::runtime_error if one of the workspaces is an invalid pointer
  */
  bool IDATab::checkWorkspaceBinningMatches(MatrixWorkspace_const_sptr left, MatrixWorkspace_const_sptr right)
  {
    if (left && right) //check the workspaces actually point to something first
    {
      auto leftX = left->readX(0);
      auto rightX = right->readX(0);
      return std::equal(leftX.begin(), leftX.end(), rightX.begin());
    }
    else
    {
      throw std::runtime_error("IDATab: One of the operands is an invalid MatrixWorkspace pointer");
    }
  }
  
  /**
   * @returns a handle to the UI form object stored in the IndirectDataAnalysis class.
   */
  Ui::IndirectDataAnalysis & IDATab::uiForm()
  {
    return m_parent->m_uiForm;
  }
  
  /**
   * @returns a const handle to the UI form object stored in the IndirectDataAnalysis class.
   */
  const Ui::IndirectDataAnalysis & IDATab::uiForm() const
  {
    return m_parent->m_uiForm;
  }

  /**
   * @returns a handle to the DoubleEditorFactory object stored in the IndirectDataAnalysis class.
   */
  DoubleEditorFactory * IDATab::doubleEditorFactory() 
  { 
    return m_parent->m_dblEdFac; 
  }

  /**
   * @returns a handle to the QtCheckBoxFactory object stored in the IndirectDataAnalysis class.
   */
  QtCheckBoxFactory * IDATab::qtCheckBoxFactory() 
  { 
    return m_parent->m_blnEdFac; 
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
