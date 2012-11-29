#ifndef MANTID_SLICEVIEWER_COMPOSITEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_COMPOSITEPEAKSPRESENTER_H_

#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtSliceViewer/NullPeaksPresenter.h"
#include <set>

namespace MantidQt
{
  namespace SliceViewer
  {
    /*---------------------------------------------------------
    CompositePeaksPresenter

    Composite implmentation of the Peaks presenter. 
    ----------------------------------------------------------*/
    class DLLExport CompositePeaksPresenter : public PeaksPresenter
    {
    public:
      
      // Overrriden methods from Peaks Presenter
      virtual void update();
      virtual void updateWithSlicePoint(const double&);
      virtual bool changeShownDim();
      virtual bool isLabelOfFreeAxis(const std::string& label) const;
      
      /// Constructor
      CompositePeaksPresenter(PeaksPresenter_sptr defaultPresenter = PeaksPresenter_sptr(new NullPeaksPresenter));
      /// Destructor
      ~CompositePeaksPresenter();
      /// Add a peaks presenter onto the composite.
      void addPeaksPresenter(PeaksPresenter_sptr presenter);
      /// Get the number of subjects.
      size_t size() const;
      /// Clear the owned presenters.
      void clear();

    private:
      /// Default behaviour 
      PeaksPresenter_sptr m_default;
      /// Subject presenters.
      std::set<PeaksPresenter_sptr> m_subjects;
      /// Use default
      bool useDefault() const { return m_subjects.size() == 0; }
    };
  }
}

#endif 