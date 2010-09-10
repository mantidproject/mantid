#ifndef MANTID_KERNEL_DATEVALIDATOR_H_
#define MANTID_KERNEL_DATEVALIDATOR_H_

#include "IValidator.h"
#include <time.h>

namespace Mantid
{
namespace Kernel
{
/** DateValidator is a validator that validates date, format of valid date is  "DD/MM/YYYY"
    At present, this validator is only available for properties of type std::string
	This class has written for validating  start and end dates of  ICat interface.
	
    @author Sofia Antony, STFC Rutherford Appleton Laboratory
    @date 03/09/2010
 
    Copyright &copy; 2010-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
	
class DLLExport DateValidator :public IValidator<std::string>
{
public:
	/// constrcutor
	DateValidator(){}
	
	/// destructor
	virtual ~DateValidator(){}
	/// create a copy of the the validator
	IValidator<std::string> * clone(){ return new  DateValidator(*this);}
private:

	/** Checks the validity of date string ,expected format is "DD/MM/YYYY"
	  * @param sDate expected date format
	  * @param error  string which describes the error
	  *return  tm structure with the date information filled in 
	*/
	struct tm getTimevalue(const std::string& sDate,std::string & error) const
	{
		struct tm  timeinfo;
		std::basic_string <char>::size_type index,off=0;
		int day,month,year;

		//look for the first '/' to extract day part from the date string
		index=sDate.find('/',off);
		if(index == std::string::npos)
		{			
			error="Invalid Date:date format must be DD/MM/YYYY";
			timeinfo.tm_mday=0;
			return timeinfo;
		}
		//get day part of the date
		day=atoi(sDate.substr(off,index-off).c_str());
		timeinfo.tm_mday=day;

		//change the offset to the next position after "/"
		off=index+1;
		//look for 2nd '/' to get month part from  the date string
		index=sDate.find('/',off);
		if(index == std::string::npos)
		{			
			error="Invalid Date:date format must be DD/MM/YYYY";
			return timeinfo;
		}
		//now get the month part
		month=atoi(sDate.substr(off,index-off).c_str());
		timeinfo.tm_mon=month-1;

		//change the offset to the position after "/"
		off=index+1;
		//now get the year part from the date string
		year=atoi(sDate.substr(off,4).c_str());

		timeinfo.tm_year=year-1900;
		timeinfo.tm_min=0;
		timeinfo.tm_sec=0;
		timeinfo.tm_hour=0;

		return timeinfo ;

	}

	/**Checks the given value is a valid date
	  *@param value input date property to validate
	  *@return a string which describes the error else ""
	*/
	std::string checkValidity(const std::string& value) const
	{ 
		//empty strings are allowed   as date is not a mandatory parameter for ICat
		if(!value.compare(""))
		{
			return "";
		}
		std::string formaterror;
		struct tm timeinfo= getTimevalue(value,formaterror);
		if(!formaterror.empty())
		{
			return formaterror;
		}

		if (timeinfo.tm_mday<1 || timeinfo.tm_mday>31)
		{
			return "Invalid Date:Day part of parameter Date must be between 1 and 31";
		}
		if (timeinfo.tm_mon<0 || timeinfo.tm_mon>11)
		{
			return "Invalid Date:Month part of parameter Date must be between 1 and 12";
		}
		//get current time
		time_t rawtime;
		time ( &rawtime );
		struct tm* currenttime = localtime ( &rawtime );
		if(timeinfo.tm_year>currenttime->tm_year)
		{
			return "Invalid Date:Year part of the parameter can not be greater than the current year";
		}
		return "";
	}

};
}
}
#endif