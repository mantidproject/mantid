#ifndef MANTID_ALGORITHMS_SETINSTRUMENTPARAMETER_H_
#define MANTID_ALGORITHMS_SETINSTRUMENTPARAMETER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

namespace Geometry
{
  class ParameterMap;
  class IComponent;
}

namespace Algorithms
{

  /** SetInstrumentParameter : A simple algorithm to add or set the value of an instrument parameter
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport SetInstrumentParameter  : public API::Algorithm
  {
  public:
    SetInstrumentParameter();
    virtual ~SetInstrumentParameter();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Add or replace an parameter attached to an instrument component.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:

    void init();
    void exec();

    void addParameter(Mantid::Geometry::ParameterMap& pmap, const Mantid::Geometry::IComponent* cmptId, const std::string& paramName, const std::string& paramType, const std::string& paramValue) const;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_SETINSTRUMENTPARAMETER_H_ */