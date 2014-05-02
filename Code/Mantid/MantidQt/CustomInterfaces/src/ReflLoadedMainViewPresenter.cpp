#include "MantidQtCustomInterfaces/ReflLoadedMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
using namespace Mantid::API;
namespace
{
  void hasValidModel(ITableWorkspace_sptr model)
  {
    std::runtime_error invalid("Selected table does not meet the specifications to become a model for this interface.");
    //TODO: wrap in try-catch
    if (model->columnCount() == 8)
    {
      model->String(0,0);
      model->String(0,1);
      model->String(0,2);
      model->String(0,3);
      model->String(0,4);
      model->String(0,5);
      model->String(0,6);
      model->Int(0,7);
    }
    else
    {
      throw invalid;
    }
  }
}
namespace MantidQt
{
  namespace CustomInterfaces
  {


    //----------------------------------------------------------------------------------------------
    /** Constructor
    */

    ReflLoadedMainViewPresenter::ReflLoadedMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view): ReflMainViewPresenter(model, view)
    {
      hasValidModel(m_model);
      load();
    }

    ReflLoadedMainViewPresenter::ReflLoadedMainViewPresenter(std::string model, ReflMainView* view): ReflMainViewPresenter(AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(model), view)
    {
      hasValidModel(m_model);
      load();
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ReflLoadedMainViewPresenter::~ReflLoadedMainViewPresenter()
    {
    }



  } // namespace CustomInterfaces
} // namespace Mantid