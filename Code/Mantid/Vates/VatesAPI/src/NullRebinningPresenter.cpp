#include "MantidVatesAPI/NullRebinningPresenter.h"
#include <vtkUnstructuredGrid.h>
#include <stdexcept>

namespace Mantid
{
  namespace VATES
  {
    NullRebinningPresenter::NullRebinningPresenter()
    {
    }

    void NullRebinningPresenter::updateModel()
    {
    }

    vtkDataSet* NullRebinningPresenter::execute(vtkDataSetFactory*, ProgressAction&, ProgressAction&)
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    const std::string& NullRebinningPresenter::getAppliedGeometryXML() const
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    NullRebinningPresenter::~NullRebinningPresenter()
    {
    }

    std::vector<double> NullRebinningPresenter::getTimeStepValues() const
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    bool NullRebinningPresenter::hasTDimensionAvailable() const
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    std::string NullRebinningPresenter::getTimeStepLabel() const
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    void NullRebinningPresenter::makeNonOrthogonal(vtkDataSet *visualDataSet)
    {
      UNUSED_ARG(visualDataSet);
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    void NullRebinningPresenter::setAxisLabels(vtkDataSet *visualDataSet)
    {
      UNUSED_ARG(visualDataSet);
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    const std::string& NullRebinningPresenter::getInstrument() const
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    double NullRebinningPresenter::getMaxValue() const
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }

    double NullRebinningPresenter::getMinValue() const
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this method. Misused");
    }
  }
}
