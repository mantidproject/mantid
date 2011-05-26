#ifndef SYNCHRONISING_GEOMETRY_PRESENTER_H_
#define SYNCHRONISING_GEOMETRY_PRESENTER_H_

#include "MantidVatesAPI/GeometryXMLParser.h"
#include "MantidVatesAPI/GeometryPresenter.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

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

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport SynchronisingGeometryPresenter : public GeometryPresenter
    {
    public:

      SynchronisingGeometryPresenter(Mantid::VATES::GeometryXMLParser& source);

      void dimensionResized(DimensionPresenter* pDimensionPresenter);

      void dimensionRealigned(DimensionPresenter* pDimensionPresenter);

      Mantid::Geometry::VecIMDDimension_sptr getNonIntegratedDimensions() const;

      std::string getGeometryXML() const;

      ~SynchronisingGeometryPresenter();

      void acceptView(GeometryView*);

      std::string getLabel(DimensionPresenter const * const pDimensionPresenter) const;

      void setModified();

    private:

      bool hasXDim() const;

      bool hasYDim() const;

      bool hasZDim() const;

      bool hasTDim() const;

      bool isXDimensionPresenter(DimensionPresenter const * const) const;

      bool isYDimensionPresenter(DimensionPresenter const * const) const;

      bool isZDimensionPresenter(DimensionPresenter const * const) const;

      bool isTDimensionPresenter(DimensionPresenter const * const) const;

      void shuffleMappedPresenters();

      void eraseMappedPresenter(DimPresenter_sptr);

      void insertMappedPresenter(DimPresenter_sptr);

      void dimensionExpanded(DimensionPresenter* pDimensionPresenter);

      void dimensionCollapsed(DimensionPresenter* pDimensionPresenter);

      /// Disabled copy constructor
      SynchronisingGeometryPresenter(const SynchronisingGeometryPresenter&);
      /// Disabled assignement operator
      SynchronisingGeometryPresenter& operator=(const SynchronisingGeometryPresenter&);
      /// The x-dimension mapping pointing to the correct dimension presenter.
      DimPresenter_sptr m_xDim;
      /// The y-dimension mapping pointing to the correct dimension presenter.
      DimPresenter_sptr m_yDim;
      /// The z-dimension mapping pointing to the correct dimension presenter.
      DimPresenter_sptr m_zDim;
      /// The t-dimension mapping pointing to the correct dimension presenter.
      DimPresenter_sptr m_tDim;
      /// Collection of synchronised non-integrated dimensions.
      Mantid::Geometry::VecIMDDimension_sptr m_notIntegrated;
      /// Collection of synchronised integrated dimensions.
      Mantid::Geometry::VecIMDDimension_sptr m_integrated;
      /// Original geometry model/source.
      Mantid::VATES::GeometryXMLParser m_source; 
      /// The View with which the presenter will be bound.
      GeometryView* m_view;
    protected:
      /// Collection of individual dimension presenters owned by this geometry presenter.
      VecDimPresenter_sptr m_dimPresenters;
    };
  }
}

#endif