#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZ_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZ_H_

#include "MantidKernel/System.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidMDEvents/ReflectometryMDTransform.h"

namespace Mantid
{
namespace MDEvents
{

  /** ReflectometryTranformQxQz : Type of ReflectometyTransform. Used to convert from an input event workspace to a 2D MDEvent workspace with dimensions of QxQy.
  Transformation is specific for reflectometry purposes.
    
    @date 2012-05-29

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
  class DLLExport ReflectometryTransformQxQz : public ReflectometryMDTransform
  {
  private:
    const double m_qxMin;
    const double m_qxMax;
    const double m_qzMin;
    const double m_qzMax;
    const double m_incidentTheta;
  public:

    /// Constructor
    ReflectometryTransformQxQz(double qxMin, double qxMax, double qzMin, double qzMax, double incidentTheta);
    /// Destructor
    ~ReflectometryTransformQxQz();
    /// Execute transformation
    virtual Mantid::API::IMDEventWorkspace_sptr execute(Mantid::API::IEventWorkspace_const_sptr eventWs) const;

  private:

    DISABLE_DEFAULT_CONSTRUCT(ReflectometryTransformQxQz)
    DISABLE_COPY_AND_ASSIGN(ReflectometryTransformQxQz)

  };



} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_REFLECTOMETRYTRANFORMQXQZ_H_ */