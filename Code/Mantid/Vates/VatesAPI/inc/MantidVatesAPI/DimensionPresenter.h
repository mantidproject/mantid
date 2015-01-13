#ifndef DIMENSION_PRESENTER_H_
#define DIMENSION_PRESENTER_H_
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidVatesAPI/GeometryPresenter.h"
#include "MantidVatesAPI/DimensionView.h"

namespace Mantid
{
  namespace VATES
  {

    /** @class DimensionPresenter

    MVP presenter for a IMDDimension model.

    @author Owen Arnold, Tessella Support Services plc
    @date 24/05/2011

    Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DimensionView;
    class DLLExport DimensionPresenter
    {
    public:
      DimensionPresenter(DimensionView* view, GeometryPresenter * geometryPresenter);
      void acceptModelStrongly(Mantid::Geometry::IMDDimension_sptr model);
      void acceptModelWeakly(Mantid::Geometry::IMDDimension_sptr model);
      void acceptAppliedModel();
      void updateModel();
      Mantid::Geometry::IMDDimension_sptr getAppliedModel() const;
      Mantid::Geometry::IMDDimension_sptr getModel() const;
      Mantid::Geometry::VecIMDDimension_sptr getNonIntegratedDimensions() const;
      std::string getVisDimensionName() const;
      std::string getLabel() const;
      void updateIfNotIntegrated();
      virtual ~DimensionPresenter();
      GeometryPresenter::MappingType getMappings() const;
      void setMapping(std::string mapping);
      std::string getMapping() const;
      void setViewMode(BinDisplay mode);
    private:
      void commonSetup();
      DimensionPresenter(const DimensionPresenter&);
      DimensionPresenter& operator=(const DimensionPresenter&);
      void validate() const;
      
      /// Core model of MVP
      Mantid::Geometry::IMDDimension_sptr m_model;

      /// Core parent geometry presenter in MVP.
      GeometryPresenter *  m_geometryPresenter; 

      /// Core MVP view.
      DimensionView* m_view; 
      
      /// Flag capturing the last state of the isIntegrated flag. Used for comparisons.
      bool m_lastIsIntegrated;
      
      /// Mapping name.
      std::string m_mapping;
    };
  }
}
#endif
