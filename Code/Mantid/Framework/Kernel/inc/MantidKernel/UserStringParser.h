#ifndef MANTID_KERNEL_RANGESTRINGPARSER_H_
#define MANTID_KERNEL_RANGESTRINGPARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/DllConfig.h"

#ifndef Q_MOC_RUN
# include <boost/tokenizer.hpp>
# include <boost/shared_ptr.hpp>
#endif

#include<vector>
namespace Mantid
{
namespace Kernel
{

  /** This class parses a given string into  vector of vectors of numbers.
    For example : 60,61+62,63-66,67:70,71-75:2  This gives a vector containing 8 vectors 
    as Vec[8] and Vec[0] is a vector containing 1 element 60
    Vec[1] is a vector containing elements 61,62 Vec[2] is a vector containing elements 63,64,65,66,
    Vec[3] is a vector containing element 67,Vec[4] is a vector containing element 68,
    Vec[5] is a vector containing element 69,Vec[6] is a vector containing element 70 ,
    vec[7] is a vector containing element 71,73,75

    @author Sofia Antony, Rutherford Appleton Laboratory
    @date 27/01/2011

    Copyright &copy; 2010-2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

  class MANTID_KERNEL_DLL UserStringParser
  {
  public:
    
    ///constructor
    UserStringParser();
    //destructor
    ~UserStringParser();
    /// parses a given string  into a vector of  vector of numbers
    std::vector<std::vector<unsigned int> > parse(const std::string& userString);
  private:
    ///separate a given string to a vector of comma separated strings
    std::vector<std::string> separateComma (const std::string&);
    /// separates a given string to vector of vector of numbers using colon as the delimeter
    std::vector<std::vector<unsigned int> > separateColon(const std::string& input);
    ///separate delimiter string from input string and return a vector of numbers created from the separated string 
    std::vector<unsigned int> separateDelimiters(const std::string& input,const std::string& symbol);
        
    /// converts a string to int.
    unsigned int  toUInt(const std::string& input);
    /// This method checks the input string contains the character ch
    bool Contains(const std::string& input,char ch);

    /// This method removes the separator string from the input string and converts the tokens to unisgned int
    void Tokenize(const std::string& input,const std::string& separator,unsigned int& start,
                   unsigned int& end,unsigned int& step);

    /// convert the string into numbers
    void parse(const std::string& userString,std::vector<std::vector<unsigned int> >& numbers);
    /// validates the input string
    bool isValid(const std::string& input,std::vector<std::string>& tokens);
    /// converts the parsed tokens to numbers
    void convertToNumbers(const std::string& input, const std::vector<std::string>& tokens,unsigned int& start, 
                           unsigned int& end, unsigned int& step);
    //returns true if the separator before the step string is valid
    bool isValidStepSeparator(const std::string& input,std::vector<std::string>& tokens);

  };

}
}
#endif
