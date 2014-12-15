#ifndef MANTID_VATES_MDHW_LOADING_PRESENTER
#define MANTID_VATES_MDHW_LOADING_PRESENTER

#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDHistoWorkspace.h"

namespace Mantid
{
  namespace VATES
  {

    /** 
    @class MDHWLoadingPresenter
    Abstract presenter encapsulating common operations used by all MDHW type loading. Reduces template bloat.
    @author Owen Arnold, Tessella plc
    @date 16/08/2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class MDLoadingView;
    class DLLExport MDHWLoadingPresenter : public MDLoadingPresenter
    {
    public:
      MDHWLoadingPresenter(MDLoadingView* view);
      const std::string& getGeometryXML() const;
      virtual bool hasTDimensionAvailable() const;
      virtual std::vector<double> getTimeStepValues() const;
      virtual std::string getTimeStepLabel() const;
      virtual void setAxisLabels(vtkDataSet* visualDataSet);
      virtual void makeNonOrthogonal(vtkDataSet* visualDataSet);
      virtual ~MDHWLoadingPresenter();
    protected:
      /*---------------------------------------------------------------------------
      Common/shared operations and members for all MDHW file-type loading.
      ---------------------------------------------------------------------------*/
      MDLoadingView* m_view;
      
      Mantid::Geometry::MDGeometryBuilderXML<Mantid::Geometry::NoDimensionPolicy> xmlBuilder;
      Mantid::Geometry::IMDDimension_sptr tDimension;
      std::vector<std::string> axisLabels;
      virtual void appendMetadata(vtkDataSet* visualDataSet,
                                  const std::string& wsName) ;
      virtual void extractMetadata(Mantid::API::IMDHistoWorkspace_sptr histoWs);
      virtual bool canLoadFileBasedOnExtension(const std::string& filename,
                                               const std::string& expectedExtension) const;
      virtual bool shouldLoad();
      bool m_isSetup;
      double m_time;
      bool m_loadInMemory;
      bool m_firstLoad;
    };

  }
}

#endif
