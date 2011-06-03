#include "MantidVatesAPI/NullRebinningPresenter.h"
#include <vtkUnstructuredGrid.h>
#include <exception>

namespace Mantid
{
  namespace VATES
  {
    NullRebinningPresenter::NullRebinningPresenter()
    {
    }

    void NullRebinningPresenter::updateModel()
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this type. Misused");
    }

    vtkUnstructuredGrid* NullRebinningPresenter::execute(ProgressAction&)
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this type. Misused");
    }

    std::string NullRebinningPresenter::getAppliedGeometryXML() const
    {
      throw std::runtime_error("NullRebinningPresenter does not implement this type. Misused");
    }

    NullRebinningPresenter::~NullRebinningPresenter()
    {
    }
  }
}