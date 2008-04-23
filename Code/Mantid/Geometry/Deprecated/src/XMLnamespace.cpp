#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <sys/stat.h>
#include <time.h>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>

#include "MantidKernel/Logger.h"
#include "AuxException.h"
#include "MantidKernel/Support.h"
#include "MantidGeometry/RegexSupport.h"
#include "MantidGeometry/Matrix.h"
#include "Vec3D.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLcomp.h"
#include "XMLcollect.h"
#include "XMLnamespace.h"

namespace Mantid
{

namespace XML
{

std::vector<std::string>
getParts(const std::string& KeyList)
  /*!
    Split the string into useful parts.
    The form is KeyA::KeyB::KeyC
    \param KeyList :: Line of the keys 
    \return string List
  */
{
  boost::regex Sep("(\\S+)::");
  std::vector<std::string> Out;
  StrFunc::StrFullSplit(KeyList,Sep,Out);
  return Out;
}

std::pair<int,std::string>
procKey(const std::string& Line)
  /*!
    Given a key : e.g. <Monitor_1>
    return the key with the folling flags 
    \param Line :: Line to pre-process
    \retval  0 :: Nothing found
    \retval  1 :: Key found (new)
    \retval -1 :: Key closed
    \retval 100 :: Key found and closed (data)
    \retval -100 :: Key found and closed (no data)
  */
    
{
  typedef std::pair<int,std::string> retType;
  boost::regex Re("<(\\S+)\\s*.*>");

  std::vector<std::string> Out;
  StrFunc::StrFullSplit(Line,Re,Out);
  std::vector<std::string>::const_iterator vc;
  if (Out.empty())
    return retType(0,"");
  if (Out.size()==1)
    {
      const std::string& Tag=Out[0];
      if (Tag[0]=='/')
	return retType(-100,Tag.substr(1));
      if (Tag[Tag.size()-1]=='/')
	return retType(-100,Tag.substr(0,Tag.size()-1));
      return retType(1,Tag);
    }
  if (Out.size()==2)
    {
      const std::string TagA=Out[0];
      const std::string TagB=Out[1];
      if (TagA==TagB.substr(0,TagB.size()-1))
	return retType(100,TagA);
    }
  std::cerr<<"Unable to decode XML:"<<Line<<std::endl;
  return retType(0,"");
}

int
getGroupContent(std::istream& FX,std::string& Key,
		std::vector<std::string>& Attrib,
		std::vector<std::string>& Data)
  /*!
    Given an open group <key> : Read until we encounter a new <grp> 
    or close a new gropu
    
    If we start from after <alpha> then continue.
    
    \param FX :: Input file stream
    \param Key :: Key name
    \param Attrib :: Attribute if found
    \retval 0 :: Failed
    \retval 1 :: opened a new object
    \retval 2 :: closed key
    \retval -1 :: Null group
  */
{

  // Clear the user provided data
  Data.clear();

  std::string Line;         // Data strings  after e.g. <key> data data .... 
  std::string Group;        // Group is the </closekey> or next group e.g. <newKey>
  char c(0);                // Character to read from file

  // Required to finish regex:
  while(FX.get(c) && c!='<')
    {
      if (c!='\n')
	Line+=c;
      else if (!Line.empty())
        {
	  Data.push_back(Line);
	  Line="";
	}
    }
  // Ok found either (a) a close group (b) a new open group
  if (!Line.empty())
    Data.push_back(Line);
  if (c!='<')
    return 0;

  while(FX.get(c) && c!='>')
    {
      if (c=='<')       // Error 
	return 0;
      Group += (c!='\n') ? c : ' ';
    }
  return procGroupString(Group,Key,Attrib);
}

int
getNextGroup(std::istream& FX,std::string& Key,
	     std::vector<std::string>& Attrib)
  /*!
    Given a file read until the next <key> or the current system closes.
    If we start from <alpha>
    \param FX :: Input file stream
    \param Key :: Key name
    \param Attrib :: Attribute if found
    \retval 0 :: Failed
    \retval 1 :: opened a new object
    \retval 2 :: closed key
    \retval -1 :: Null group
  */
{
  std::string Group;
  char c(0);   // Character to read from file

  int init(0);
  int quote=0;
  // Required to finish regex:
  while(FX.get(c) && 
	(c!='>' || quote || !init)) 
    {

      if (c=='\"')                  // Quote
	quote=1-quote;
      Group+=c;
      if (c=='<' && !quote)         //start Point
        {
	  Group="";
	  init=1;
	}
    }
  return procGroupString(Group,Key,Attrib);
}

int
procGroupString(const std::string& Group,
	     std::string& Key,
	     std::vector<std::string>& AtVec)
  /*!
    Split a group  <..... >  into keys and attributes
    This will have a part <\key>
    \param Group :: Group we are after
    \param Key :: Key value found
    \param Attr :: Return value of 

    \retval 0 :: Failed
    \retval 1 :: opened a new object
    \retval 2 :: closed key
    \retval -1 :: Null group
  */
{
  AtVec.clear();
  // Found Attribute and normal key : <Key Att=this att2=this>
  std::string Part=Group;
  
  std::string Kval;
  if (!StrFunc::section(Part,Kval))
    return -0;
  if (Kval[0]=='/')           // closed group
    {
      Key=Kval.substr(1);
      return 2;
    }
  Key=Kval;
  // Note the reuse of the string
  while(StrFunc::section(Part,Kval))
    {
      AtVec.push_back(Kval);
    }
  if (Kval[Kval.length()-1]=='/')
    {
      if (AtVec.empty())
	Key=Kval.substr(0,Kval.length()-1);
      else
	AtVec.back()=Kval.substr(0,Kval.length()-1);
      return -1;
    }

  return 1;
}

int
collectBuffer(std::istream& FX,std::vector<std::string>& Buffer)
  /*!
    Collect a buffer of lines from FX between 
    the components.
    \param FX :: Filestream
    \param Buffer :: vector of strings to find
    \return value
   */
{
  Buffer.clear();
  std::string Line;
  char c;
  while (FX.good() && FX.get(c) && c!='<')
    {
      if (c=='\n')
        {
	  if (!Line.empty())
            {
	      Buffer.push_back(Line);
	      Line="";
	    }
	}
      else
	Line+=c;
    }
  return Buffer.size();
}

int
splitComp(std::istream& FX,
	  std::string& closeKey,
	  std::string& Line)
  /*!
    After a <key> for a component is read
    read until <\key> closes the system
    and pass the line object
    \param closeKey :: <key/> that closed things
    \retval 0 :: Success
    \retval -1 :
   */
{
  // Now read body (until close)
  char c;
  while(FX.get(c) && c!='<')
    Line+=c;
  // Need a </key> to close the group
  if (!FX.get(c) || c!='/')
    return -1;
  while(FX.get(c) && c!='>')
    closeKey+=c;
  return 0;
}

int
splitLine(std::istream& FX,
	  std::string& closeKey,
	  std::string& Line)
  /*!
    After a <key> for a component is read
    read until <\key> closes the system
    and pass the line object.
    Only reads one Line . 
    \param closeKey :: <key/> that closed things
    \retval 0 :: Success
    \retval 1 :: finished
    \retval -1 :: failed
   */
{
  // Now read body (until close)
  char c;
  while(FX.get(c) && c!='<' && c!='\n')
    Line+=c;
  if (c=='\n')
    return 0;
  // Need a </key> to close the group
  if (!FX.get(c) || c!='/')
    return -1;
  while(FX.get(c) && c!='>')
    closeKey+=c;
  return 1;
}

int
splitGroup(std::istream& FX,std::string& Key,
	   std::vector<std::string>& Attrib,std::string& Body)
  /*!
    From a file: read <key attrib=.....> Body <key/>
    \param FX :: Filestream
    \param Key :: Key of group
    \param Attrib :: Attributes (if any)
    \param Body :: Main component
    \retval 0 :: failure
    \retval 1 :: success
  */
{
  // Note the extra .*? at the front :: 
  // This removes all the pre-junk.
  const int flag = getNextGroup(FX,Key,Attrib);
  if (!flag || flag==2)
    return 0;
  if (flag==-1)
    return 1;
  // Now read body (until close)
  char c;
  std::string Line;
  while(FX.good())
    {
      while(FX.get(c) && c!='<' && c!='>')
        {
	  Line+=c;
	}
      if (c=='>' && Line=="/"+Key)
	return 1;
      Body+=Line;
      Line="";
    }
  return 0;
}

int
getFilePlace(std::istream& FX,const std::string& KeyList)
  /*!
    Determine a place in a file given a string
    \param FX :: Filestream 
    \param KeyList :: Key list in the form A::B::C
    \retval -1 :: Empty Tag
    \retval 1 :: Opening tag
    \retval 0 :: nothing found
  */
{
  const int keyLen(KeyList.length());
  std::string keyVal;
  std::vector<std::string> Attrib;
  int len;
  std::string FullName,testStr;
  std::string::size_type pos;
  while (FX.good())
    {
      const int flag=getNextGroup(FX,keyVal,Attrib);
      switch (flag)
        {
	case 0:
	  return 0;
	case 1:            // Open new
	  FullName+="::"+keyVal;
	  len=FullName.length();
	  if (len>=keyLen)
	    {
	      testStr=FullName.substr(len-keyLen,std::string::npos);
	      if (testStr==KeyList)
		return 1;
	    }
	  break;
	case 2:
	  pos=FullName.rfind("::");
	  if (pos==std::string::npos)
	    return 0;
	  FullName.erase(pos);
	  break;
	case -1:
	  testStr=FullName+"::"+keyVal;
	  len=testStr.length();
	  testStr=testStr.substr(len-keyLen,std::string::npos);
	  if (testStr==KeyList)
	    return -1;
	  break;
	default:
	  std::cerr<<"Error in swith of XMLnamespace"<<std::endl;
	}
    }
  return 0;
}

int
getNumberIndex(const std::multimap<std::string,int>& MX,
	       const std::string& Key)
  /*!
    Carry out a binary search to determine the lowest
    value that can be indexed into MX . 
    - Tested in testXML::testBinSearch
    \param MX :: Map to search
    \param Key :: BaseKey to add numbers to. e.g. Alpha
    \return 0 :: success 
  */
{
  int index=1;
  int step=1;
  int lowBound(0);
  int highBound(-1);
  std::multimap<std::string,int>::const_iterator mc;

  // Boundary step:
  std::ostringstream cx;
  while(highBound<0)
    {
      cx.str("");
      cx<<Key<<index;
      mc=MX.find(cx.str());
      if (mc!=MX.end())
	lowBound=index;
      else 
	highBound=index;
      index+=step;
      step*=2;
    }
  // Search step
  while(highBound-lowBound>1)
    {
      const int mid=(highBound+lowBound)/2;
      cx.str("");
      cx<<Key<<mid;
      mc=MX.find(cx.str());
      if (mc!=MX.end())
	lowBound=mid;
      else 
	highBound=mid;
    }
  return highBound;
}

int
splitAttribute(std::string& AList,std::string& Key,std::string& Value)
  /*!
    Given an attribute list e.g.
    - File="Test" Out="junk" 
    Split into components
    \retval 0 if nothing to do
    \retval -1 error
    \retval 1 :: success 
   */
{
  std::string::size_type pos=AList.find("=");     
  if (pos==std::string::npos)
    return 0;
  StrFunc::convert(AList.substr(0,pos),Key);
  AList.erase(0,pos+1);
  if (!XML::cutString(AList,Value))
    return -1;
  return 1;
}


int
cutString(std::string& AList,std::string& Value)
  /*!
    Given a string with quotes e.g. File="cut"
    It takes out the "cut" part.

    \param AList :: String to find " " section and cut out
    \param Value :: Value to put
    \retval -ve :: Error with Length
    \retval Length of string extracted (possibly 0)
   */
{
  unsigned int start;
  unsigned int end;

  for(start=0;start<AList.length() && AList[start]!='"';start++);
  for(end=start+1;end<AList.length() && AList[end]!='"';end++);
  if (end>=AList.length())
    return -1;

  Value=AList.substr(start+1,end-(start+1));
  AList.erase(start,1+start-end);
  return end-(start+1);
}

std::string
procString(const std::string& Item)
  /*!
    Given a string Item 
    produce the string without the standard 
    symbol that XML requires out
    \todo Use a proper Entities class
  */
{

  std::string::size_type pos=Item.find_first_of("&<>");
  std::ostringstream cx;
  std::string::size_type cutPos=0;
  while(pos!=std::string::npos)
    {
      cx<<Item.substr(cutPos,pos-cutPos);
      switch (Item[pos])
      {
	case '<':
	  cx<<"&lt";
	  break;
	case '>':
	  cx<<"&gt";
	  break;
	case '&':
	  cx<<"&amp";
	  break;
      }
      cutPos=pos+1;
      pos=Item.find_first_of("&<>",pos);
    }
  cx<<Item.substr(cutPos,std::string::npos);
  return cx.str();
}

int
matchPath(const std::string& A,const std::string& B)
  /*!
    Check that A and B match: B can contain a regular 
    expression.
    \param A :: Primary Key
    \param B :: Secondary Key / Regex
    \retval 1 :: Match (exact)
    \retval 2 :: Match [with regex]
   */
{
  if (A==B)
    return 1;
  boost::regex Re(B);
  if (StrFunc::StrLook(A,Re))
    return 2;
  return 0;
}

}  // Namespace XML

}  // Namespace Mantid
