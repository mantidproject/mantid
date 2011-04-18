#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include "MantidGeometry/Math/RotCounter.h"

namespace Mantid
{

namespace Geometry
{
std::ostream&
operator<<(std::ostream& OX,const RotaryCounter& A) 
  /**
    Output stream assesor
    @param OX :: Output stream
    @param A :: RotaryCounter to writeout
    @return the ouput stream
   */
{
  A.write(OX);
  return OX;
}


RotaryCounter::RotaryCounter(const int S,const int N) :
  Rmax(N),RC(S)
  /**
    Simple constructor with fixed size and number.
    Fills RC with a flat 0->N number list
    @param S :: Size  (number of components)
    @param N :: Max number to get to
  */
{
  for(int i=0;i<S;i++)
    RC[i]=i;
}

RotaryCounter::RotaryCounter(const RotaryCounter& A) :
  Rmax(A.Rmax),RC(A.RC)
  /**
    Standard copy constructor
    @param A :: Object to copy
  */
{ }

RotaryCounter&
RotaryCounter::operator=(const RotaryCounter& A)
  /**
    Assignment operator
    @param A :: Object to copy
    @return *this
   */
{
  if (this!=&A)
    {
      Rmax=A.Rmax;
      RC=A.RC;
    }
  return *this;
}

RotaryCounter::~RotaryCounter()
  /**
    Standard Destructor
  */
{}

int
RotaryCounter::operator==(const RotaryCounter& A) const
  /**
    Chec to find if Counters identical in ALL respects
    @param A :: Counter to compare
    @retval 1 :: All things identical
    @retval 0 :: Something not the same
  */
{
  if (RC.size()!=A.RC.size())
    return 0;

  for(size_t i=0;i<RC.size();i++)
  {
    if (RC[i]!=A.RC[i])
    {
      return 0;
    }
  }
  return 1;
}

int
RotaryCounter::operator>(const RotaryCounter& A) const 
  /** 
    Determines the precidence of the RotaryCounters
    Operator works on the 0 to high index 
    @param A :: RotaryCounter to compare
    @return This > A
   */
{
  const size_t ourSize = RC.size();
  const size_t theirSize = A.RC.size();
  const size_t maxI = (theirSize > ourSize) ? ourSize :theirSize;
  for(size_t i=0;i<maxI;i++)
  {
    if (RC[i]!=A.RC[i])
    {
      return RC[i]>A.RC[i];
    }
  }
  if (theirSize !=ourSize)
  {
    return static_cast<int>(ourSize > theirSize);
  }
  return 0;
}

int
RotaryCounter::operator<(const RotaryCounter& A) const
  /** 
    Determines the precidence of the RotaryCounters
    Operator works on the 0 to high index 
    @param A :: RotaryCounter to compare
    @return This < A
   */
{
  const size_t ourSize = RC.size();
  const size_t theirSize = A.RC.size();
  const size_t maxI = (theirSize>ourSize) ? ourSize : theirSize;
  for(size_t i=0;i<maxI;i++)
  {
    if (RC[i]!=A.RC[i])
    {
      return RC[i]<A.RC[i];
    }
  }
  if (theirSize!=ourSize)
    return ourSize < theirSize;
  return 0;
}

int 
RotaryCounter::operator++(int a)
  /**
    Convertion to ++operator (prefix) 
    from operator++ (postfix)
    @param a :: ignored
    @return ++operator
   */
{
  (void) a; //Avoid compiler warning
  return this->operator++();
}

int
RotaryCounter::operator++()
  /**
    Carrys out a rotational addition.
    Objective is a rolling integer stream ie 1,2,3
    going to 1,2,N-1 and then 1,3,4 etc...
    @retval 1 :: the function has looped (carry flag)
    @retval 0 :: no loop occored
  */
{
  int Npart=Rmax-1;
  int I;
  for(I=static_cast<int>(RC.size())-1;I>=0 && RC[I]==Npart;I--,Npart--);
  
  if (I<0)
    {
      for(int i=0;i<static_cast<int>(RC.size());i++)
	RC[i]=i;
      return 1;
    }
  RC[I]++;
  for(I++;I<static_cast<int>(RC.size());I++)
    RC[I]=RC[I-1]+1;
  return 0;
}

int 
RotaryCounter::operator--(int a)
  /**
    convertion to --operator (prefix) 
    from operator-- (postfix)
    @param a :: ignored
    @return --operator
   */
{
  (void) a; //Avoid compiler warning
  return this->operator--();
}

int
RotaryCounter::operator--()
  /**
    Carrys out a rotational addition.
    Objective is a rooling integer stream ie 1,2,3
    going to 1,2,N-1 and then 1,3,4 etc...
    @retval 1 :: the function has looped (carry flag)
    @retval 0 :: no loop occored
  */
{
  const int Size(static_cast<int>(RC.size()));
  int I;
  for(I=Size-1;I>0 && RC[I]==RC[I-1]+1;I--);
  // Loop case
  if(!I && !RC[0])    
    {
      // In case of loop go to
      for(int i=0;i<Size;i++)
      {
      	RC[i]=Rmax+i-Size;
      }
      return 1;
    }
  
  RC[I]--;
  for(I++;I<Size;I++)
  {
    RC[I]=Rmax+I-Size;
  }
  return 0;
}

void
RotaryCounter::write(std::ostream& OX) const
  /**
    Write out object to a stream
    @param OX :: output stream
  */
{
  OX<<" ";
  copy(RC.begin(),RC.end()-1,std::ostream_iterator<int>(OX,":"));
  OX<<RC.back()<<" ";
  return;
}
}
}
