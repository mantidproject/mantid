#ifndef MANTID_DATAOBJECTS_ALGORITHMHISTORY_H_
#define MANTID_DATAOBJECTS_ALGORITHMHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmParameter.h"
#include <ctime>
#include <vector>

namespace Mantid
{
  namespace API
  {

    /** @class AlgorithmHistory AlgorithmHistory.h API/MAntidAPI/AlgorithmHistory.h

    This class stores information about the Command History used by algorithms on a workspace.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
    class DLLExport AlgorithmHistory
    {
    public:
      /// The date-and-time will be stored as the ctime time_t type
      typedef time_t dateAndTime;
      AlgorithmHistory();
      AlgorithmHistory(const std::string&, const int&, 
        const dateAndTime&, const double&,
        const std::vector<AlgorithmParameter>&);
      virtual ~AlgorithmHistory();
      AlgorithmHistory& operator=(const AlgorithmHistory&);
      AlgorithmHistory(const AlgorithmHistory&);
      // get functions
      /// get name of algorithm in history const
      const std::string& name()const {return m_name;}
      /// get version number of algorithm in history const
      const int& version()const {return m_version;}
      /// get parameter list of algorithm in history const
      const std::vector<AlgorithmParameter>& getParameters() const {return m_parameters;}
      /// print contents of object
      void printSelf(std::ostream&,const int indent = 0)const;
      
      //this is required for boost.python
      bool operator==(const AlgorithmHistory &other) const 
      {
	      if (name() == other.name() && version() == other.version() && getParameters() == other.getParameters())
	      {
	      	      return true;
	      }
	      
	      return false;
      }

    private:
      /// The name of the Algorithm
      std::string m_name;
      /// The version of the algorithm
      int m_version;
      /// The execution date of the algorithm
      dateAndTime m_executionDate;
      /// The execution duration of the algorithm
      double m_executionDuration;
      /// The AlgorithmParameter's defined for each the algorithm
      std::vector<AlgorithmParameter> m_parameters;
    };

    DLLExport std::ostream& operator<<(std::ostream&, const AlgorithmHistory&);

  } // namespace API
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_ALGORITHMHISTORY_H_*/
