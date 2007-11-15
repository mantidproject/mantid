#ifndef RotaryCounter_h
#define RotaryCounter_h

/*!
  \class RotaryCounter
  \version 1.0
  \date September 2005
  \author S. Ansell
  \brief Simple multilevel-cyclic counter

  Objective is a rolling integer stream ie 1,2,3
  going to 1,2,N-1 and then 1,3,4 etc...
*/

class RotaryCounter
{
  friend std::ostream& operator<<(std::ostream&,const RotaryCounter&);

 private:

  int Rmax;                 ///< Number to over cycle
  std::vector<int> RC;      ///< rotation list

 public:
  
  RotaryCounter(const int,const int);  ///<Size,Max
  RotaryCounter(const RotaryCounter&);
  RotaryCounter& operator=(const RotaryCounter&);
  ~RotaryCounter();

  int operator==(const RotaryCounter&) const;
  int operator<(const RotaryCounter&) const;
  int operator>(const RotaryCounter&) const;
  /// Accessor operator
  int operator[](const int I) const { return RC[I]; }
  int operator++();
  int operator++(const int);
  int operator--();
  int operator--(const int);
  
  void write(std::ostream&) const;
};

std::ostream& operator<<(std::ostream&,const RotaryCounter&);


#endif
