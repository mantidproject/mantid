//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/UserStringParser.h"
#include <boost/lexical_cast.hpp>

namespace Mantid
{
  namespace Kernel
  {
    ///constructor
    UserStringParser::UserStringParser()
    {
    }
    ///Destructor
    UserStringParser::~UserStringParser()
    {

    }
    
  /**This method parses a given string of numbers and returns a vector of vector of numbers.
    *@param userString - the string to parse
    *@returns  a vector containing vectors of numbers.
   */
    std::vector<std::vector<unsigned int> > UserStringParser::parse(const std::string& userString)
    {
     std::vector<std::vector<unsigned int> > numbers;
      //first separate commas
      std::vector<std::string> commaseparatedstrings;
      if(userString.find(",")!=std::string::npos)
      {
        commaseparatedstrings=separateComma(userString);
      }
      if(!commaseparatedstrings.empty())
      {     
        std::vector<std::string>::const_iterator citr;
        for(citr=commaseparatedstrings.begin();citr!=commaseparatedstrings.end();++citr)
        { 
          parse((*citr),numbers);
        }

      }
      else
      {
        parse(userString,numbers);
      }
      return numbers;
      
    }

  /**This method parses a given string of numbers and returns a vector of vector of numbers.
    *@param userString - the input  string to parse
    *@param numbers- a vector containing vectors of numbers.
   */
    void  UserStringParser::parse(const std::string& userString,
                                              std::vector<std::vector<unsigned int> >& numbers)
    {

      //look for  separators
      std::string separators("-+:");
      std::vector<unsigned int>value;
      //if input contains no separator string 
      if(userString.find_first_of(separators)==std::string::npos)
      { 
        numbers.push_back(std::vector<unsigned int>(1,toUInt(userString)));
      }
      else if(Contains(userString,'-'))
      {
        std::vector<unsigned int>value =separateDelimiters(userString,"-:"); 
        if(!value.empty())
        {
          numbers.push_back(value);
        }
      }
      else if (Contains(userString,'+'))
      {
        std::vector<unsigned int>value =separateDelimiters(userString,"+");
        if(!value.empty())
        {
          numbers.push_back(value);
        }
      }
      else if (Contains(userString,':'))
      {
        std::vector<std::vector<unsigned int> >colonseparated= separateColon(userString);
        std::vector<std::vector<unsigned int> >::const_iterator citr1;
        for(citr1=colonseparated.begin();citr1!=colonseparated.end();++citr1)
        {
          numbers.push_back((*citr1));
        }
      }
    
    }

  /** This method checks input string contains character ch 
    * @param input - the input string
    * @param ch - character ch to search
    * @returns - true if the string contains character ch.
    */
    bool UserStringParser::Contains(const std::string& input,char ch)
    {
      std::string::size_type pos = input.find(ch);
      return (pos==std::string::npos?false:true);
    }

  /**This method parses a given string of numbers into comma separated tokens.
    *@param input - the string to parse
    *@returns  a vector containing comma separated tokens.
    */
    std::vector<std::string> UserStringParser::separateComma(const std::string& input)
    {
     
      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
      boost::char_separator<char> commasep(",");
       std::vector<std::string> commaseparatedvalues;
      
      tokenizer tokens(input,commasep);
      for(tokenizer::iterator tokItr=tokens.begin();tokItr!=tokens.end();++tokItr)
      {       
          commaseparatedvalues.push_back(*tokItr);
      }
      return commaseparatedvalues;
         
    }

   /**This method parses a given string of numbers into colon separated tokens.
     *@param input - the string to parse
     *@returns  a vector of vector containing colon separated tokens.
    */
    std::vector<std::vector<unsigned int> > UserStringParser::separateColon(const std::string& input)
    {     
      unsigned int startNum=0;
      unsigned int endNum=0;
      unsigned int step=1;
      std::vector<std::vector<unsigned int> > separatedValues;
      Tokenize(input,":",startNum,endNum,step);
      for(unsigned int num=startNum;num<=endNum;num+=step)
      {
        separatedValues.push_back(std::vector<unsigned int>(1,num));
      }

      return separatedValues;

    }

   /**This method parses a given string of numbers into tokens using the separator character symbol.
     *@param input - the string to parse
     *@param delimiters - the string used as separator
     *@returns  a vector of vector containing colon separated tokens.
    */
    std::vector<unsigned int> UserStringParser::separateDelimiters(const std::string& input,const std::string& delimiters)
    {     
      unsigned int startNum=0;
      unsigned int endNum=0;
      unsigned int step=1;
      std::vector<unsigned int> separatedValues;
      Tokenize(input,delimiters,startNum,endNum,step);
      
      for(unsigned int num=startNum;num<=endNum;num+=step)
      {
        separatedValues.push_back(num);
      }
      return separatedValues;

    }
  
  /**This method parses a given string of numbers into tokens based on the given separator
    *returns numbers corresponding to separated tokens and separators if any
    *@param input - the string to parse
    *@param delimiter - character to separate
    *@param start - a number corresponding to the string before delimiter
    *@param end -  a number corresponding to the string after delimiter
    *@param step - number used to increment from the start num generate the series of numbers . 
    */
    void UserStringParser::Tokenize(const std::string& input,const std::string& delimiter,
      unsigned int& start, unsigned int& end,unsigned int& step)
    {
      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
      
      boost::char_separator<char> seps(delimiter.c_str());
      std::vector<unsigned int> separatedValues;
      std::vector<std::string> temp;
      tokenizer tokens(input,seps);
      for(tokenizer::const_iterator tokItr=tokens.begin();tokItr!=tokens.end();++tokItr)
      {
        temp.push_back(*tokItr);
        
      }
      if(temp.empty())
      {
        return ;
      }
      //validate the separated tokens
      if(!isValidStepSeparator(input,temp))
      {
        throw std::runtime_error("Non supported format found in the input string "+input+ " Step string should be preceded by :");
      }
      //convert the parsed string to number
      convertToNumbers(input,temp,start,end,step);
  
    }

    /**This method checks the separator preceded by the step string is valid
      * colon ':' is valid separator for step string
      *@param input - input string to validate
      *@param tokens - vector containing separated values.
      *@returns true if the input is valid
      */
    bool UserStringParser::isValidStepSeparator(const std::string& input,std::vector<std::string>& tokens)
    {
      std::string step_separator;
      if(tokens.size()==3)
      {
        std::string step=tokens[2];

        std::string::size_type index=input.rfind(step);
        if(index!=std::string::npos)
        {
          step_separator=input.substr(index-1,1);
        }
        //step values must be preceded by colon ':'
        return(!step_separator.compare(":")?true:false);
      }
      return true;
      
    }

  /**This method checks the input string is valid format
    *@param input - input string to validate
    *@param tokens - vector containing separated values.
    *@param start  - start number 
    *@param end    - end number 
    *@param step   - step used for incrementing
    */
    void UserStringParser::convertToNumbers(const std::string& input,const std::vector<std::string>& tokens,
                                            unsigned int& start, unsigned int& end, unsigned int& step)
    {
      if(tokens.empty())
      {
        return;
      }
      try
      {
        start = toUInt(tokens.at(0)); 
        end= toUInt(tokens.at(1)); 
        if(tokens.size()==3)
        {          
          step=toUInt(tokens.at(2)); 
        }
        if(end<start)
        {
          throw std::runtime_error("Invalid Input String: End number "+ tokens.at(1)+" can not be lower than start number"+ tokens.at(0));
        }
        if(start+step>end)
        {
          throw std::runtime_error("Invalid Input String: End number "+ tokens.at(1)+ " can not be lower than the sum of start number "+ tokens.at(0) +" and step number" + tokens.at(2));
        }
      }
      catch(std::runtime_error& e)
      {
        throw std::runtime_error(e.what());
      }
      catch(std::out_of_range&)
      {
        throw std::runtime_error("Error when parsing the input string "+input);
      }
      

    }
  /**This method converts a string to unsigned int
    *@param input - the string to parse
    *@returns an unsigned number equivalent of the input
    */
    unsigned int  UserStringParser::toUInt(const std::string& input)
    {
      try
      {
        return boost::lexical_cast<unsigned int>(input);
      }
      catch(boost::bad_lexical_cast&)
      {
        throw std::runtime_error("Error when parsing the input  string "+input);
      }
    }

    
  }
}
