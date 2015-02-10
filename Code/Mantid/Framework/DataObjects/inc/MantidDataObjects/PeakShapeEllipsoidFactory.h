#ifndef MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORY_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORY_H_

#include "MantidKernel/System.h"
#include "PeakShapeFactory.h"

namespace Mantid
{
namespace DataObjects
{

  /** PeakShapeEllipsoidFactory : Create ellipsoid peak shapes

    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport PeakShapeEllipsoidFactory : public PeakShapeFactory
  {
  public:
    PeakShapeEllipsoidFactory();
    virtual ~PeakShapeEllipsoidFactory();
       
    // PeakShapeFactory interface

    Mantid::Geometry::PeakShape *create(const std::string &source) const;
    void setSuccessor(boost::shared_ptr<const PeakShapeFactory> successorFactory);

  private:
    /// Successor factory
    PeakShapeFactory_const_sptr m_successor;
  };


} // namespace DataObjects
} // namespace Mantid

#endif  /* MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORY_H_ */
