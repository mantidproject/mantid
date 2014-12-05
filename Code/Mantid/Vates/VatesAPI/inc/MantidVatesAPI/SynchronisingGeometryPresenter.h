#ifndef SYNCHRONISING_GEOMETRY_PRESENTER_H_
#define SYNCHRONISING_GEOMETRY_PRESENTER_H_

#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidVatesAPI/GeometryPresenter.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidVatesAPI/DimensionView.h"

namespace Mantid
{
  namespace VATES
  {
    //Forward dec
    class DimensionPresenter;

    typedef boost::shared_ptr<DimensionPresenter> DimPresenter_sptr;
    
    typedef std::vector<DimPresenter_sptr> VecDimPresenter_sptr;

    /** @class SynchronisingGeometryPresenter

    Concrete type for MVP style presenter for a Multi-dimensional workspace geometry. This implementation synchronises changes between non-integrated and integrated dimensions.
    contains knowledge on what should happen as non-integrated dimensions are collapsed and vica-versa.

    @author Owen Arnold, Tessella Support Services plc
    @date 24/05/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport SynchronisingGeometryPresenter : public GeometryPresenter
    {
    public:

      SynchronisingGeometryPresenter(Mantid::Geometry::MDGeometryXMLParser& source);

      void dimensionResized(DimensionPresenter* pDimensionPresenter);

      void dimensionRealigned(DimensionPresenter* pDimensionPresenter);

      Mantid::Geometry::VecIMDDimension_sptr getNonIntegratedDimensions() const;

      Mantid::Geometry::VecIMDDimension_sptr getIntegratedDimensions() const;

      MappingType getMappings() const;

      std::string getGeometryXML() const;

      ~SynchronisingGeometryPresenter();

      void acceptView(GeometryView*);

      void setModified();

      void setDimensionModeChanged();

      //Constant reference name for an X-AXIS
      const std::string X_AXIS;
      //Constant reference name for an Y-AXIS
      const std::string Y_AXIS;
      //Constant reference name for an Z-AXIS
      const std::string Z_AXIS;
      //Constant reference name for an T-AXIS
      const std::string T_AXIS;

    private:

      void swap(const GeometryPresenter::MappingType::key_type& keyA, const GeometryPresenter::MappingType::key_type& keyB);

      bool hasXDim() const;

      bool hasYDim() const;

      bool hasZDim() const;

      bool hasTDim() const;

      bool isXDimensionPresenter(DimPresenter_sptr dimensionPresenter) const;

      bool isYDimensionPresenter(DimPresenter_sptr dimensionPresenter) const;

      bool isZDimensionPresenter(DimPresenter_sptr dimensionPresenter) const;

      bool isTDimensionPresenter(DimPresenter_sptr dimensionPresenter) const;

      void shuffleMappedPresenters();

      void eraseMappedPresenter(DimPresenter_sptr);

      void insertMappedPresenter(DimPresenter_sptr);

      void dimensionExpanded(DimensionPresenter* pDimensionPresenter);

      /// Disabled copy constructor
      SynchronisingGeometryPresenter(const SynchronisingGeometryPresenter&);
      /// Disabled assignement operator
      SynchronisingGeometryPresenter& operator=(const SynchronisingGeometryPresenter&);
      /// Collection of synchronised non-integrated dimensions.
      mutable Mantid::Geometry::VecIMDDimension_sptr m_dimensions;
      /// Original geometry model/source.
      Mantid::Geometry::MDGeometryXMLParser m_source; 
      /// The View with which the presenter will be bound.
      GeometryView* m_view;
      /// Map containing pairs of visdimensionnames to dimension presenters.
      MappingType m_mapping;
      /// Current bin display mode 
      BinDisplay m_binDisplayMode;
    protected:

      virtual void dimensionCollapsed(DimensionPresenter* pDimensionPresenter);

      /// Collection of individual dimension presenters owned by this geometry presenter.
      VecDimPresenter_sptr m_dimPresenters;
    };
  }
}

#endif