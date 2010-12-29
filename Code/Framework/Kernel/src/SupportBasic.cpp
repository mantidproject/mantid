#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>

#include "MantidKernel/Support.h"

namespace Mantid
{

namespace StrFunc
{

/*!
  Function to convert a number into hex
  output (and leave the stream un-changed)
  \param OFS :: Output stream
  \param n :: Integer to convert
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

/*!
  Removes the multiple spaces in the line
  \param Line :: Line to process
  \return String with single space components
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

/*!
  Checks that as least cnt letters of 
  works is part of the string. It is currently 
  case sensative. It removes the Word if found
  \param Line :: Line to process
  \param Word :: Word to use
  \param cnt :: Length of Word for significants [default =4]
  \retval 1 on success (and changed Line) 
  \retval 0 on failure 
*/
int extractWord(std::string& Line,const std::string& Word,const int cnt)
{
  if (Word.empty())
    return 0;

  unsigned int minSize(cnt>static_cast<int>(Word.size()) ?  Word.size() : cnt);
  std::string::size_type pos=Line.find(Word.substr(0,minSize));
  if (pos==std::string::npos)
    return 0;
  // Pos == Start of find
  unsigned int LinePt=minSize+pos;
  for(;minSize<Word.size() && LinePt<Line.size()
	&& Word[minSize]==Line[LinePt];LinePt++,minSize++)
  {
  }

  Line.erase(pos,LinePt-(pos-1));
  return 1;
}

/*!
  Check to see if S is the same as the
  first part of a phrase. (case insensitive)
  \param S :: string to check
  \param fullPhrase :: complete phrase
  \returns 1 on success 
*/
int confirmStr(const std::string& S,const std::string& fullPhrase)
{
  const int nS(S.length());
  const int nC(fullPhrase.length());
  if (nS>nC || nS<=0)    
    return 0;           
  for(int i=0;i<nS;i++)
    if (S[i]!=fullPhrase[i])
      return 0;
  return 1;
}

/*!
  Gets a line and determine if there is addition component to add
  in the case of a very long line.
  \param fh :: input stream to get line 
  \param Out :: string up to last 'tab' or ' '
  \param Excess :: string after 'tab or ' ' 
  \param spc :: number of char to try to read 
  \retval 1 :: more line to be found
  \retval -1 :: Error with file
  \retval 0  :: line finished.
*/
int getPartLine(std::istream& fh,std::string& Out,std::string& Excess,const int spc)
{
  std::string Line;
  if (fh.good())
    {
      char* ss=new char[spc+1];
      const int clen=spc-Out.length();
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

/*!
  Removes all spaces from a string 
  except those with in the form '\ '
  \param CLine :: Line to strip
  \return String without space
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
	
/*!
  Reads a line from the stream of max length spc.
  Trailing comments are removed. (with # or ! character)
  \param fh :: already open file handle
  \param spc :: max number of characters to read 
  \return String read.
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

/*!
  Determines if a string is only spaces
  \param A :: string to check
  \returns 1 on an empty string , 0 on failure
*/
int isEmpty(const std::string& A)
{
  std::string::size_type pos=
    A.find_first_not_of(" \t");
  return (pos!=std::string::npos) ? 0 : 1;
}

/*!
  removes the string after the comment type of 
  '$ ' or '!' or '#  '
  \param A :: String to process
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

/*!
  Returns the string from the first non-space to the 
  last non-space 
  \param A :: string to process
  \returns shortened string
*/
std::string fullBlock(const std::string& A)
{
  std::string::size_type posA=A.find_first_not_of(" ");
  std::string::size_type posB=A.find_last_not_of(" ");
  if (posA==std::string::npos)
    return "";
  return A.substr(posA,1+posB-posA);
}


/*!
  Write out the line in the limited form for MCNPX
  ie initial line from 0->72 after that 8 to 72
  (split on a space or comma)
  \param Line :: full MCNPX line
  \param OX :: ostream to write to
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

/*!
  Splits the sting into parts that are space delminated.
  \param Ln :: line component to strip
  \returns vector of components
*/
std::vector<std::string> StrParts(std::string Ln)
{
  std::vector<std::string> Out;
  std::string Part;
  while(section(Ln,Part))
    Out.push_back(Part);
  return Out;
}

/*!
  Converts a vax number into a standard unix number
  \param A :: float number as read from a VAX file
  \returns float A in IEEE little eindian format
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

}  // NAMESPACE StrFunc

}  // NAMESPACE Mantid
