#ifndef StrFunc_SupportTempCode_h
#define StrFunc_SupportTempCode_h

#include <string.h>

namespace Mantid
{
namespace  StrFunc
{

/*!
  Takes a character string and evaluates 
  the first [typename T] object. The string is then 
  erase upt to the end of number.
  The diffierence between this and section is that
  it allows trailing characters after the number. 
  \param out :: place for output
  \param A :: string to process
  \returns 1 on success 0 on failure
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
  const size_t xpt=static_cast<size_t>(cx.tellg());
  if (xpt<0)
    return 0;
  A.erase(0,xpt);
  out=retval;
  return 1; 
}

/*!
  Takes a character string and evaluates 
  the first [typename T] object. The string is then filled with
  spaces upto the end of the [typename T] object
  \param out :: place for output
  \param cA :: char array for input and output. 
  \returns 1 on success 0 on failure
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
  \param out :: place for output
  \param A :: string for input and output. 
  \return 1 on success 0 on failure
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
  const size_t xpt=static_cast<size_t>(cx.tellg());
  const char xc=cx.get();
  if (!cx.fail() && !isspace(xc))
    return 0;
  A.erase(0,xpt);
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
  \param out :: place for output
  \param A :: string for input and output. 
  \return 1 on success 0 on failure
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
      const size_t xpt=static_cast<size_t>(cx.tellg());
      const char xc=cx.get();
      if (!cx.fail() && !isspace(xc) 
	  && (xc!='-' || xpt<5))
	return 0;
      A.erase(0,xpt);
      out=retval;
      return 1;
    }
  return 0;
}

/*!
  Takes a character string and evaluates 
  the first [typename T] object. The string is then 
  erase upto the end of number.
  The diffierence between this and convert is that
  it allows trailing characters after the number. 
  \param out :: place for output
  \param A :: string to process
  \retval number of char read on success
  \retval 0 on failure
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
  const size_t xpt=static_cast<size_t>(cx.tellg());
  if (xpt<0)
    return 0;
  out=retval;
  return xpt; 
}

/*!
  Convert a string into a value 
  \param A :: string to pass
  \param out :: value if found
  \returns 0 on failure 1 on success
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

/*!
  Convert a string into a value 
  \param A :: string to pass
  \param out :: value if found
  \returns 0 on failure 1 on success
*/
template<typename T>
int convert(const char* A,T& out)
{
  // No string, no conversion
  if (!A) return 0;
  std::string Cx=A;
  return convert(Cx,out);
}

/*!
  Write out the three vectors into a file of type dc 9
  \param step :: parameter to control x-step (starts from zero)
  \param Y :: Y column
  \param Fname :: Name of the file
  \returns 0 on success and -ve on failure
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

/*!
  Write out the three vectors into a file of type dc 9
  \param X :: X column
  \param Y :: Y column
  \param Fname :: Name of the file
  \returns 0 on success and -ve on failure
*/
template<template<typename T,typename A> class V,typename T,typename A> 
int writeFile(const std::string& Fname,const V<T,A>& X,const V<T,A>& Y)
{
  V<T,A> Ex;   // Empty vector/list
  return writeFile(Fname,X,Y,Ex);  // don't need to specific ??
}

/*!
  Write out the three container into a file with
  column free-formated data in the form :
   - X  Y Err
   If Err does not exist (or is short) 0.0 is substituted.
  \param X :: X column
  \param Y :: Y column
  \param Err :: Err column
  \param Fname :: Name of the file
  \returns 0 on success and -ve on failure
*/
template<template<typename T,typename A> class V,typename T,typename A> 
int writeFile(const std::string& Fname,const V<T,A>& X,const V<T,A>& Y,const V<T,A>& Err)
{
  const int Npts(X.size()>Y.size() ? Y.size() : X.size());
  const int Epts(Npts>static_cast<int>(Err.size()) ? Err.size() : Npts);

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

/*!  
  Call to read in various values in position x1,x2,x3 from the
  line. Note to avoid the dependency on crossSort this needs
  to be call IN ORDER 
  \param Line :: string to read
  \param Index :: Indexes to read
  \param Out :: OutValues [unchanged if not read]
  \retval 0 :: success
  \retval -ve on failure.
*/
template<typename T> 
int setValues(const std::string& Line,const std::vector<int>& Index,std::vector<T>& Out)
{
  if (Index.empty())
    return 0;
  
  if(Out.size()!=Index.size())
    return -1;
//    throw ColErr::MisMatch<int>(Index.size(),Out.size(),
//				"StrFunc::setValues");

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
	    return -1-distance(static_cast<iVecIter>(sIndex.begin()),sc);  
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
	    return -1-distance(static_cast<iVecIter>(sIndex.begin()),sc);  
	}
      cnt++;         // Add only to cnt [sc/oc in while loop]
    }
  // Success since loop only gets here if sc is exhaused.
  return 0;       
}

}  // NAMESPACE StrFunc

}  // NAMESPACE Mantid


#endif
