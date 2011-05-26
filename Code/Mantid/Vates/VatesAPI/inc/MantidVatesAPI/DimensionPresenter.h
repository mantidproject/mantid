#ifndef DIMENSION_PRESENTER_H_
#define DIMENSION_PRESENTER_H_
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
namespace Mantid
{
  namespace VATES
  {
    //Forward declarations.
    class GeometryPresenter;
    class DimensionView;

    /** @class DimensionPresenter.

    MVP presenter for a IMDDimension model.

    @author Owen Arnold, Tessella Support Services plc
    @date 24/05/2011

    Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport DimensionPresenter
    {
    public:
      DimensionPresenter(DimensionView* view, GeometryPresenter * geometryPresenter);
      void acceptModel(Mantid::Geometry::IMDDimension_sptr model);
      void acceptAppliedModel();
      void updateModel();
      Mantid::Geometry::IMDDimension_sptr getAppliedModel() const;
      Mantid::Geometry::IMDDimension_sptr getModel() const;
      Mantid::Geometry::VecIMDDimension_sptr getNonIntegratedDimensions() const;
      std::string getDimensionId() const;
      std::string getLabel() const;
      void updateIfNotIntegrated();
      virtual ~DimensionPresenter();
    private:
      DimensionPresenter(const DimensionPresenter&);
      DimensionPresenter& operator=(const DimensionPresenter&);
      Mantid::Geometry::IMDDimension_sptr m_model;
      GeometryPresenter *  m_geometryPresenter; 
      DimensionView* m_view; 
      void validate() const;
      bool m_lastIsIntegrated;
    };
  }
}
#endif