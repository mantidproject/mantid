#ifndef MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_
#define MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/Detector.h"

namespace Mantid
{
namespace Algorithms
{

  /** NormaliseByDetector : TODO: DESCRIPTION
    
    @date 2012-07-17

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
  class DLLExport NormaliseByDetector  : public API::Algorithm
  {
  public:
    NormaliseByDetector();
    virtual ~NormaliseByDetector();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    const Mantid::Geometry::FitParameter tryParseFunctionParameter(Mantid::Geometry::Parameter_sptr parameter, Geometry::IDetector_const_sptr det);
    virtual void initDocs();
    void init();
    void exec();


  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_ */