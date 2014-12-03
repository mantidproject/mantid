#ifndef MANTID_VATES_MDEW_LOADING_PRESENTER
#define MANTID_VATES_MDEW_LOADING_PRESENTER

#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDEventWorkspace.h"

#include <boost/scoped_ptr.hpp>

namespace Mantid
{
  namespace VATES
  {

    /** 
    @class MDEWLoadingPresenter
    Abstract presenter encapsulating common operations used by all MDEW type loading. Reduces template bloat.
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
    class DLLExport MDEWLoadingPresenter : public MDLoadingPresenter
    {
    public:
      MDEWLoadingPresenter(MDLoadingView* view);
      const std::string& getGeometryXML() const;
      virtual bool hasTDimensionAvailable() const;
      virtual std::vector<double> getTimeStepValues() const;
      virtual std::string getTimeStepLabel() const;
      virtual void setAxisLabels(vtkDataSet* visualDataSet);
      virtual ~MDEWLoadingPresenter();
      virtual const std::string& getInstrument();
      virtual double getMinValue();
      virtual double getMaxValue();

    protected:
      /*---------------------------------------------------------------------------
      Common/shared operations and members for all MDEW file-type loading.
      ---------------------------------------------------------------------------*/
      MDLoadingView* m_view;
      Mantid::Geometry::MDGeometryBuilderXML<Mantid::Geometry::NoDimensionPolicy> xmlBuilder;

      boost::scoped_ptr<MetadataJsonManager> m_metadataJsonManager;
      boost::scoped_ptr<MetaDataExtractorUtils> m_metaDataExtractor;
      boost::scoped_ptr<VatesConfigurations> m_vatesConfigurations;

      Mantid::Geometry::IMDDimension_sptr tDimension;

      std::vector<std::string> axisLabels;
      virtual void appendMetadata(vtkDataSet* visualDataSet, const std::string& wsName) ;
      virtual void extractMetadata(Mantid::API::IMDEventWorkspace_sptr eventWs);
      virtual bool canLoadFileBasedOnExtension(const std::string& filename, const std::string& expectedExtension) const;
      virtual bool shouldLoad();
      bool m_isSetup;
      double m_time;
      size_t m_recursionDepth;
      bool m_loadInMemory;
      bool m_firstLoad;
    };
  }
}

#endif
