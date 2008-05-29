#ifndef MANTIDAPI_LOCATEDDATAVALUE_H
#define MANTIDAPI_LOCATEDDATAVALUE_H

#include "MantidKernel/System.h"
#include "MantidAPI/ILocatedData.h"

namespace Mantid
{

  namespace API
  {

    //forward declaration
    class IErrorHelper;

    /**
    IDataItem of an X value, two error values E and E2 together with a pointer to an ErrorHelper and a specta number.
    
    \author N. Draper

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    */
    class DLLExport LocatedDataValue : public ILocatedData
    {
    public:

      const double& X() const;
      const double& E() const;
      const double& E2() const;
      const IErrorHelper* ErrorHelper() const;

      double& X();
      double& E();
      double& E2();

      const double& Y() const;
      double& Y();

      double xValue;        ///< value of X
      double yValue;        ///< value of Y
      double eValue;        ///< value of E
      double e2Value;       ///< value of E2
      const IErrorHelper* errorHelper; ///< Pointer to the approriate error helper

      const double& X2() const;
      double& X2();

      const bool isHistogram() const; ///<Returns true if the data point is hastogram data and therefore has an X2.

      double x2Value;        ///< value of X2

      LocatedDataValue();
      LocatedDataValue(const LocatedDataValue&);
      LocatedDataValue(const ILocatedData&);
      LocatedDataValue& operator=(const LocatedDataValue&);
      LocatedDataValue& operator=(const ILocatedData&);
      virtual ~LocatedDataValue();

      int operator<(const LocatedDataValue&) const;
      int operator>(const LocatedDataValue&) const;
      int operator==(const LocatedDataValue&) const;
      int operator!=(const LocatedDataValue&) const;
    private:
      bool _isHistogram; ///< True if the data is a histogram
    };

  }  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_LOCATEDDATAVALUE_H
