#ifndef MANTIDAPI_POINTDATAVALUE_H
#define MANTIDAPI_POINTDATAVALUE_H

#include "MantidKernel/System.h"
#include "MantidAPI/IPointData.h"

namespace Mantid
{

namespace API
{

  //forward declaration
  class IErrorHelper;

/**
  The PointDataValue class holds references to all of the data required to describe a single data item of a point dataset.
  All of the data items are held within this class than itself.  
  This is used to prevent lifetime issues when returning data from calculations using PointDataRef classes.

  \author N. Draper
    
  Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
  class DLLExport PointDataValue : public IPointData
{
  public:
  const double& X() const;
  const double& E() const;
  const double& E2() const;
  const IErrorHelper* ErrorHelper() const;
  int SpectraNo() const;
  
  double& X();
  double& E();
  double& E2();

  const double& Y() const;
  double& Y();

  double xValue;        ///< value of X
  double yValue;        ///< value of Y
  double eValue;        ///< value of E
  double e2Value;       ///< value of E2
  int spectraNo;
  const IErrorHelper* errorHelper;

  PointDataValue* clone() const;

  PointDataValue();
  PointDataValue(const PointDataValue&);
  PointDataValue(const IPointData&);
  PointDataValue& operator=(const PointDataValue&);
  PointDataValue& operator=(const IPointData&);
  virtual ~PointDataValue();

  int operator<(const PointDataValue&) const;
  int operator>(const PointDataValue&) const;
  int operator==(const PointDataValue&) const;
  int operator!=(const PointDataValue&) const;
};

}  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_POINTDATAVALUE_H
