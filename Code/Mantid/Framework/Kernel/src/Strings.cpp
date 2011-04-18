#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <string.h>

#include "MantidKernel/Strings.h"

using std::size_t;

namespace Mantid
{
namespace Kernel
{
namespace Strings
{

//------------------------------------------------------------------------------------------------
/** Return a string with all matching occurence-strings
 *
 * @param input :: input string
 * @param find_what :: will search for all occurences of this string
 * @param replace_with :: ... and replace them with this.
 * @return the modified string.
 */
std::string replace(const std::string input, const std::string find_what, const std::string replace_with)
{
  std::string output = input;
  std::string::size_type pos=0;
  while((pos=output.find(find_what, pos))!=std::string::npos)
  {
    output.erase(pos, find_what.length());
    output.insert(pos, replace_with);
    pos+=replace_with.length();
  }
  return output;
}


//------------------------------------------------------------------------------------------------
/**
  Function to convert a number into hex
  output (and leave the stream un-changed)
  @param OFS :: Output stream
  @param n :: Integer to convert
  \todo Change this to a stream operator
*/
void printHex(std::ostream& OFS,const int n)
{
  std::ios_base::fmtflags PrevFlags=OFS.flags();
  OFS<<"Ox";
  OFS.width(8);
  OFS.fill('0');
  hex(OFS);
  OFS << n;
  OFS.flags(PrevFlags);
  return;
} 

//------------------------------------------------------------------------------------------------
/**
  Removes the multiple spaces in the line
  @param Line :: Line to process
  @return String with single space components
*/
std::string stripMultSpc(const std::string& Line)
{
  std::string Out;
  int spc(1);
  int lastReal(-1);
  for(unsigned int i=0;i<Line.length();i++)
    {
      if (Line[i]!=' ' && Line[i]!='\t' &&
    		Line[i]!='\r' &&  Line[i]!='\n')		
        {
	  lastReal=i;
	  spc=0;
	  Out+=Line[i];
	}
      else if (!spc)
        {
	  spc=1;
	  Out+=' ';
	}
    }
  lastReal++;
  if (lastReal<static_cast<int>(Out.length()))
    Out.erase(lastReal);
  return Out;
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
/**
  Checks that as least cnt letters of 
  works is part of the string. It is currently 
  case sensative. It removes the Word if found
  @param Line :: Line to process
  @param Word :: Word to use
  @param cnt :: Length of Word for significants [default =4]
  @retval 1 on success (and changed Line) 
  @retval 0 on failure 
*/
int extractWord(std::string& Line,const std::string& Word,const int cnt)
{
  if (Word.empty())
    return 0;

  size_t minSize(cnt>static_cast<int>(Word.size()) ?  Word.size() : cnt);
  std::string::size_type pos=Line.find(Word.substr(0,minSize));
  if (pos==std::string::npos)
    return 0;
  // Pos == Start of find
  size_t LinePt=minSize+pos;
  for(;minSize<Word.size() && LinePt<Line.size()
	&& Word[minSize]==Line[LinePt];LinePt++,minSize++)
  {
  }

  Line.erase(pos,LinePt-(pos-1));
  return 1;
}

//------------------------------------------------------------------------------------------------
/**
  Check to see if S is the same as the
  first part of a phrase. (case insensitive)
  @param S :: string to check
  @param fullPhrase :: complete phrase
  @return 1 on success 
*/
int confirmStr(const std::string& S,const std::string& fullPhrase)
{
  const size_t nS(S.length());
  const size_t nC(fullPhrase.length());
  if (nS>nC || nS<=0)    
    return 0;           
  for(size_t i=0;i<nS;i++)
    if (S[i]!=fullPhrase[i])
      return 0;
  return 1;
}

//------------------------------------------------------------------------------------------------
/**
  Gets a line and determine if there is addition component to add
  in the case of a very long line.
  @param fh :: input stream to get line 
  @param Out :: string up to last 'tab' or ' '
  @param Excess :: string after 'tab or ' ' 
  @param spc :: number of char to try to read 
  @retval 1 :: more line to be found
  @retval -1 :: Error with file
  @retval 0  :: line finished.
*/
int getPartLine(std::istream& fh,std::string& Out,std::string& Excess,const int spc)
{
  std::string Line;
  if (fh.good())
    {
      char* ss=new char[spc+1];
      const int clen = static_cast<int>(spc-Out.length());
      fh.getline(ss,clen,'\n');
      ss[clen+1]=0;           // incase line failed to read completely
      Out+=static_cast<std::string>(ss);
      delete [] ss;                   
      // remove trailing comments
      std::string::size_type pos = Out.find_first_of("#!");        
      if (pos!=std::string::npos)
        {
	  Out.erase(pos); 
	  return 0;
	}
      if (fh.gcount()==clen-1)         // cont line
        {
	  pos=Out.find_last_of("\t ");
	  if (pos!=std::string::npos)
	    {
	      Excess=Out.substr(pos,std::string::npos);
	      Out.erase(pos);
	    }
	  else
	    Excess.erase(0,std::string::npos);
	  fh.clear();
	  return 1;
	}
      return 0;
    }
  return -1;
}

//------------------------------------------------------------------------------------------------
/**
  Removes all spaces from a string 
  except those with in the form '\ '
  @param CLine :: Line to strip
  @return String without space
*/
std::string removeSpace(const std::string& CLine)
{
  std::string Out;
  char prev='x';
  for(unsigned int i=0;i<CLine.length();i++)
    {
      if (!isspace(CLine[i]) || prev=='\\')
        {
	  Out+=CLine[i];
	  prev=CLine[i];
	}
    }
  return Out;
}
	
//------------------------------------------------------------------------------------------------
/**
  Reads a line from the stream of max length spc.
  Trailing comments are removed. (with # or ! character)
  @param fh :: already open file handle
  @param spc :: max number of characters to read 
  @return String read.
*/
std::string getLine(std::istream& fh,const int spc)
{
  char* ss=new char[spc+1];
  std::string Line;
  if (fh.good())
    {
      fh.getline(ss,spc,'\n');
      ss[spc]=0;           // incase line failed to read completely
      Line=ss;
      // remove trailing comments
      std::string::size_type pos = Line.find_first_of("#!");
      if (pos!=std::string::npos)
	Line.erase(pos); 
    }
  delete [] ss;
  return Line;
}

//------------------------------------------------------------------------------------------------
/**
  Determines if a string is only spaces
  @param A :: string to check
  @return 1 on an empty string , 0 on failure
*/
int isEmpty(const std::string& A)
{
  std::string::size_type pos=
    A.find_first_not_of(" \t");
  return (pos!=std::string::npos) ? 0 : 1;
}

//------------------------------------------------------------------------------------------------
/**
  removes the string after the comment type of 
  '$ ' or '!' or '#  '
  @param A :: String to process
*/
void stripComment(std::string& A)
{
  std::string::size_type posA=A.find("$ ");
  std::string::size_type posB=A.find("# ");
  std::string::size_type posC=A.find("!");
  if (posA>posB)
    posA=posB;
  if (posA>posC)
    posA=posC;
  if (posA!=std::string::npos)
    A.erase(posA,std::string::npos);
  return;
}

//------------------------------------------------------------------------------------------------
/**
  Returns the string from the first non-space to the 
  last non-space 
  @param A :: string to process
  @return shortened string
*/
std::string fullBlock(const std::string& A)
{
  return strip(A);
}

//------------------------------------------------------------------------------------------------
/**  Returns the string from the first non-space to the
     last non-space
  @param A :: string to process
  @return shortened string
*/
std::string strip(const std::string& A)
{
  std::string::size_type posA=A.find_first_not_of(" ");
  std::string::size_type posB=A.find_last_not_of(" ");
  if (posA==std::string::npos)
    return "";
  return A.substr(posA,1+posB-posA);
}


//------------------------------------------------------------------------------------------------
/**
  Write out the line in the limited form for MCNPX
  ie initial line from 0->72 after that 8 to 72
  (split on a space or comma)
  @param Line :: full MCNPX line
  @param OX :: ostream to write to
*/
void writeMCNPX(const std::string& Line,std::ostream& OX)
{
  const int MaxLine(72);
  std::string::size_type pos(0);
  std::string X=Line.substr(0,MaxLine);
  std::string::size_type posB=X.find_last_of(" ,");
  int spc(0);
  while (posB!=std::string::npos && 
	 static_cast<int>(X.length())>=MaxLine-spc)
    {
      pos+=posB+1;
      if (!isspace(X[posB]))
	posB++;
      const std::string Out=X.substr(0,posB);
      if (!isEmpty(Out))
        {
	  if (spc)
	    OX<<std::string(spc,' ');
	  OX<<X.substr(0,posB)<<std::endl;
	}
      spc=8;
      X=Line.substr(pos,MaxLine-spc);
      posB=X.find_last_of(" ,");
    }
  if (!isEmpty(X))
    {
      if (spc)
	OX<<std::string(spc,' ');
      OX<<X<<std::endl;
    }
  return;
}

//------------------------------------------------------------------------------------------------
/**
  Splits the sting into parts that are space delminated.
  @param Ln :: line component to strip
  @return vector of components
*/
std::vector<std::string> StrParts(std::string Ln)
{
  std::vector<std::string> Out;
  std::string Part;
  while(section(Ln,Part))
    Out.push_back(Part);
  return Out;
}

//------------------------------------------------------------------------------------------------
/**
  Converts a vax number into a standard unix number
  @param A :: float number as read from a VAX file
  @return float A in IEEE little eindian format
*/
float getVAXnum(const float A) 
{
  union 
   {
     char a[4];
     float f;
     int ival;
   } Bd;

  int sign,expt,fmask;
  float frac;
  double onum;

  Bd.f=A;
  sign  = (Bd.ival & 0x8000) ? -1 : 1;
  expt = ((Bd.ival & 0x7f80) >> 7);   //reveresed ? 
  if (!expt) 
    return 0.0;

  fmask = ((Bd.ival & 0x7f) << 16) | ((Bd.ival & 0xffff0000) >> 16);
  expt-=128;
  fmask |=  0x800000;
  frac = (float) fmask  / 0x1000000;
  onum= frac * sign * 
         pow(2.0,expt);
  return (float) onum;
}


//------------------------------------------------------------------------------------------------
/**
  Takes a character string and evaluates
  the first [typename T] object. The string is then
  erase upt to the end of number.
  The diffierence between this and section is that
  it allows trailing characters after the number.
  @param out :: place for output
  @param A :: string to process
  @return 1 on success 0 on failure
 */
template<typename T>
int sectPartNum(std::string& A,T& out)
{
  if (A.empty())
    return 0;

  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  const std::streamoff xpt = cx.tellg();
  if (xpt < 0)
    return 0;
  A.erase(0,static_cast<unsigned int>(xpt));
  out=retval;
  return 1;
}

//------------------------------------------------------------------------------------------------
/**
  Takes a character string and evaluates
  the first [typename T] object. The string is then filled with
  spaces upto the end of the [typename T] object
  @param out :: place for output
  @param cA :: char array for input and output.
  @return 1 on success 0 on failure
 */
template<typename T>
int section(char* cA,T& out)
{
  if (!cA) return 0;
  std::string sA(cA);
  const int item(section(sA,out));
  if (item)
    {
      strcpy(cA,sA.c_str());
      return 1;
    }
  return 0;
}

/*
  takes a character string and evaluates
  the first <T> object. The string is then filled with
  spaces upto the end of the <T> object
  @param out :: place for output
  @param A :: string for input and output.
  @return 1 on success 0 on failure
*/
template<typename T>
int section(std::string& A,T& out)
{
  if (A.empty()) return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  if (cx.fail())
    return 0;
  const std::streamoff xpt = cx.tellg();
  const char xc=cx.get();
  if (!cx.fail() && !isspace(xc))
    return 0;
  A.erase(0, static_cast<unsigned int>(xpt));
  out=retval;
  return 1;
}

/*
  Takes a character string and evaluates
  the first [T] object. The string is then filled with
  spaces upto the end of the [T] object.
  This version deals with MCNPX numbers. Those
  are numbers that are crushed together like
  - 5.4938e+04-3.32923e-6
  @param out :: place for output
  @param A :: string for input and output.
  @return 1 on success 0 on failure
*/
template<typename T>
int sectionMCNPX(std::string& A,T& out)
{
  if (A.empty()) return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  if (!cx.fail())
    {
      const std::streamoff xpt = cx.tellg();
      if( xpt < 0 )
      {
        return 0;
      }
      const char xc=cx.get();
      if (!cx.fail() && !isspace(xc) && (xc!='-' || xpt<5))
      {
        return 0;
      }
      A.erase(0, static_cast<unsigned int>(xpt));
      out=retval;
      return 1;
    }
  return 0;
}

//------------------------------------------------------------------------------------------------
/**
  Takes a character string and evaluates
  the first [typename T] object. The string is then
  erase upto the end of number.
  The diffierence between this and convert is that
  it allows trailing characters after the number.
  @param out :: place for output
  @param A :: string to process
  @retval number of char read on success
  @retval 0 on failure
 */
template<typename T>
int convPartNum(const std::string& A,T& out)
{
  if (A.empty()) return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  const std::streamoff xpt = cx.tellg();
  if (xpt<0)
    return 0;
  out=retval;
  return static_cast<int>(xpt);
}

//------------------------------------------------------------------------------------------------
/**
  Convert a string into a value
  @param A :: string to pass
  @param out :: value if found
  @return 0 on failure 1 on success
*/
template<typename T>
int convert(const std::string& A,T& out)
{
  if (A.empty()) return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  if (cx.fail())
    return 0;
  const char clast=cx.get();
  if (!cx.fail() && !isspace(clast))
    return 0;
  out=retval;
  return 1;
}

//------------------------------------------------------------------------------------------------
/**
  Convert a string into a value
  @param A :: string to pass
  @param out :: value if found
  @return 0 on failure 1 on success
*/
template<typename T>
int convert(const char* A,T& out)
{
  // No string, no conversion
  if (!A) return 0;
  std::string Cx=A;
  return convert(Cx,out);
}

//------------------------------------------------------------------------------------------------
/**
  Write out the three vectors into a file of type dc 9
  @param step :: parameter to control x-step (starts from zero)
  @param Y :: Y column
  @param Fname :: Name of the file
  @return 0 on success and -ve on failure
*/
template<template<typename T,typename A> class V,typename T,typename A>
int writeFile(const std::string& Fname,const T step, const V<T,A>& Y)
{
  V<T,A> Ex;   // Empty vector
  V<T,A> X;    // Empty vector
  for(unsigned int i=0;i<Y.size();i++)
    X.push_back(i*step);

  return writeFile(Fname,X,Y,Ex);
}

//------------------------------------------------------------------------------------------------
/**
  Write out the three vectors into a file of type dc 9
  @param X :: X column
  @param Y :: Y column
  @param Fname :: Name of the file
  @return 0 on success and -ve on failure
*/
template<template<typename T,typename A> class V,typename T,typename A>
int writeFile(const std::string& Fname,const V<T,A>& X,const V<T,A>& Y)
{
  V<T,A> Ex;   // Empty vector/list
  return writeFile(Fname,X,Y,Ex);  // don't need to specific ??
}

//------------------------------------------------------------------------------------------------
/**
  Write out the three container into a file with
  column free-formated data in the form :
   - X  Y Err
   If Err does not exist (or is short) 0.0 is substituted.
  @param X :: X column
  @param Y :: Y column
  @param Err :: Err column
  @param Fname :: Name of the file
  @return 0 on success and -ve on failure
*/
template<template<typename T,typename A> class V,typename T,typename A>
int writeFile(const std::string& Fname,const V<T,A>& X,const V<T,A>& Y,const V<T,A>& Err)
{
  const size_t Npts(X.size()>Y.size() ? Y.size() : X.size());
  const size_t Epts(Npts > Err.size() ? Err.size() : Npts);

  std::ofstream FX;

  FX.open(Fname.c_str());
  if (!FX.good())
    return -1;

  FX<<"# "<<Npts<<" "<<Epts<<std::endl;
  FX.precision(10);
  FX.setf(std::ios::scientific,std::ios::floatfield);
  typename V<T,A>::const_iterator xPt=X.begin();
  typename V<T,A>::const_iterator yPt=Y.begin();
  typename V<T,A>::const_iterator ePt=(Epts ? Err.begin() : Y.begin());

  // Double loop to include/exclude a short error stack
  int eCount=0;
  for(;eCount<Epts;eCount++)
    {
      FX<<(*xPt)<<" "<<(*yPt)<<" "<<(*ePt)<<std::endl;
      xPt++;
      yPt++;
      ePt++;
    }
  for(;eCount<Npts;eCount++)
    {
      FX<<(*xPt)<<" "<<(*yPt)<<" 0.0"<<std::endl;
      xPt++;
      yPt++;
    }
  FX.close();
  return 0;
}

//------------------------------------------------------------------------------------------------
/**
  Call to read in various values in position x1,x2,x3 from the
  line. Note to avoid the dependency on crossSort this needs
  to be call IN ORDER
  @param Line :: string to read
  @param Index :: Indexes to read
  @param Out :: OutValues [unchanged if not read]
  @retval 0 :: success
  @retval -ve on failure.
*/
template<typename T>
int setValues(const std::string& Line,const std::vector<int>& Index,std::vector<T>& Out)
{
  if (Index.empty())
    return 0;

  if(Out.size()!=Index.size())
    return -1;
//    throw ColErr::MisMatch<int>(Index.size(),Out.size(),
//        "Mantid::Kernel::Strings::setValues");

  std::string modLine=Line;
  std::vector<int> sIndex(Index);     // Copy for sorting
  std::vector<int> OPt(Index.size());
  for(unsigned int i=0;i<Index.size();i++)
    OPt[i]=i;


  //  mathFunc::crossSort(sIndex,OPt);

  typedef std::vector<int>::const_iterator iVecIter;
  std::vector<int>::const_iterator sc=sIndex.begin();
  std::vector<int>::const_iterator oc=OPt.begin();
  int cnt(0);
  T value;
  std::string dump;
  while(sc!=sIndex.end() && *sc<0)
    {
      sc++;
      oc++;
    }

  while(sc!=sIndex.end())
    {
      if (*sc==cnt)
        {
    if (!section(modLine,value))
      return static_cast<int>(-1-distance(static_cast<iVecIter>(sIndex.begin()),sc));
    // this loop handles repeat units
    do
      {
        Out[*oc]=value;
        sc++;
        oc++;
      } while (sc!=sIndex.end() && *sc==cnt);
  }
      else
        {
    if (!section(modLine,dump))
      return static_cast<int>(-1-distance(static_cast<iVecIter>(sIndex.begin()),sc));
  }
      cnt++;         // Add only to cnt [sc/oc in while loop]
    }
  // Success since loop only gets here if sc is exhaused.
  return 0;
}



/// \cond TEMPLATE

template DLLExport int section(std::string&,double&);
template DLLExport int section(std::string&,float&);
template DLLExport int section(std::string&,int&);
template DLLExport int section(std::string&,std::string&);
//template DLLExport int section(std::string&, Mantid::Geometry::V3D&);

template DLLExport int sectPartNum(std::string&,double&);
template DLLExport int sectPartNum(std::string&,int&);
template DLLExport int sectionMCNPX(std::string&,double&);

template DLLExport int convert(const std::string&,double&);
template DLLExport int convert(const std::string&,std::string&);
template DLLExport int convert(const std::string&,int&);
template DLLExport int convert(const char*,std::string&);
template DLLExport int convert(const char*,double&);
template DLLExport int convert(const char*,int&);

template DLLExport int convPartNum(const std::string&,double&);
template DLLExport int convPartNum(const std::string&,int&);

template DLLExport int setValues(const std::string&,const std::vector<int>&,std::vector<double>&);

template DLLExport int writeFile(const std::string&,const double,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<double>&,const std::vector<double>&,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<double>&,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<float>&,const std::vector<float>&);
template DLLExport int writeFile(const std::string&,const std::vector<float>&,const std::vector<float>&,const std::vector<float>&);

/// \endcond TEMPLATE

}  // NAMESPACE Strings

}  // Namespace Kernel

}  // NAMESPACE Mantid
