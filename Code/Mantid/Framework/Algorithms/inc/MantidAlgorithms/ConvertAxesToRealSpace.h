#ifndef MANTID_ALGORITHMS_CONVERTAXESTOREALSPACE_H_
#define MANTID_ALGORITHMS_CONVERTAXESTOREALSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

#include <map>

namespace Mantid
{

namespace Algorithms
{
  
  /** ConvertAxesToRealSpace : Converts the spectrum and TOF axes to real space values, integrating the data in the process
    
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
  class DLLExport ConvertAxesToRealSpace  : public API::Algorithm
  {
  public:
    ConvertAxesToRealSpace();
    virtual ~ConvertAxesToRealSpace();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;

  private:
    void init();
    void exec();

    ///Local cache data about a spectra
    struct SpectraData
    {
      double verticalValue;
      double horizontalValue;
      double intensity;
      double error;
      int verticalIndex;
      int horizontalIndex;
    };

    ///summary data about an Axis
    struct AxisData
    {
      std::string label;
      double min;
      double max;
      int bins;
    };

    //map to store unit captions and measures
    std::map < std::string,std::string > m_unitMap;
    
    void fillAxisValues(MantidVec& vector, const AxisData& axisData, bool isHistogram);
    void fillUnitMap(std::vector<std::string>& orderedVector, std::map <std::string,std::string>& unitMap, const std::string& caption, const std::string& unit);

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_CONVERTAXESTOREALSPACE_H_ */