#ifndef RotaryCounter_h
#define RotaryCounter_h

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Logger.h"

namespace Mantid {

namespace Geometry {
/**
  \class RotaryCounter
  \version 1.0
  \date September 2005
  \author S. Ansell
  \brief Simple multilevel-cyclic counter

  Objective is a rolling integer stream ie 1,2,3
  going to 1,2,N-1 and then 1,3,4 etc...
*/

class MANTID_GEOMETRY_DLL RotaryCounter {
  friend std::ostream &operator<<(std::ostream & /*OX*/,
                                  const RotaryCounter & /*A*/);

private:
  int Rmax;            ///< Number to over cycle
  std::vector<int> RC; ///< rotation list

public:
  RotaryCounter(const int S, const int N); ///<Size,Max
  RotaryCounter(const RotaryCounter & /*A*/);
  RotaryCounter &operator=(const RotaryCounter & /*A*/);
  ~RotaryCounter();

  int operator==(const RotaryCounter & /*A*/) const;
  int operator<(const RotaryCounter & /*A*/) const;
  int operator>(const RotaryCounter & /*A*/) const;
  /// Accessor operator
  int operator[](const int I) const { return RC[I]; }
  int operator++();
  int operator++(const int a);
  int operator--();
  int operator--(const int a);

  void write(std::ostream & /*OX*/) const;
};

std::ostream &operator<<(std::ostream & /*OX*/, const RotaryCounter & /*A*/);
}
}

#endif
