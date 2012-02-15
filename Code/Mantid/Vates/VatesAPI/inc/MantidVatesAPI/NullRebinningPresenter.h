#ifndef MANTID_VATES_NULL_REBINNING_PRESENTER
#define MANTID_VATES_NULL_REBINNING_PRESENTER
#include "MantidVatesAPI/MDRebinningPresenter.h"
namespace Mantid
{
  namespace VATES
  {
    class DLLExport NullRebinningPresenter : public MDRebinningPresenter
    {
    public:

      NullRebinningPresenter();

      virtual void updateModel();

      virtual vtkDataSet* execute(vtkDataSetFactory*, ProgressAction&, ProgressAction&);

      virtual const std::string& getAppliedGeometryXML() const;

      virtual std::vector<double> getTimeStepValues() const;

      virtual bool hasTDimensionAvailable() const;

      virtual ~NullRebinningPresenter();

    private:

      NullRebinningPresenter(const NullRebinningPresenter&);

      NullRebinningPresenter& operator=(const NullRebinningPresenter&);

    };
  }
}

#endif
