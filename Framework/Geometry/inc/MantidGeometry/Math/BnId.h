// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef BNID_H
#define BNID_H

#include "MantidGeometry/DllConfig.h"
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Mantid {

namespace Geometry {
/**
  \class  BnId
  \brief Tri-state variable
  \author S. Ansell
  \date April 2005
  \version 1.0


  This class holds a tri-state variable
  of -1 (false) 0 (not-important) 1 (true) against
  each of the possible input desisions. It has
  arbitrary lenght (unlike using a long integer)
  \todo Could be improved by using a set of
  unsigned integers.


*/

class MANTID_GEOMETRY_DLL BnId {
private:
  size_t size;           ///< number of variables
  int PI;                ///< Prime Implicant
  int Tnum;              ///< True number (1 in Tval)
  int Znum;              ///< Zero number (0 in Tval)
  std::vector<int> Tval; ///< Truth values

  void setCounters(); ///< Calculates Tnum and Znum

public:
  BnId();
  BnId(const size_t, unsigned int);

  bool operator==(const BnId &) const; ///< Equals operator for tri-state object
  bool operator<(const BnId &) const;  ///< operator> for tri-state object
  bool operator>(const BnId &) const;  ///< operator> for tri-state object
  int operator[](int const) const;     ///< Access operator
  int operator++(int); ///< addition operator (returns !carry flag)
  int operator++();    ///< addition operator (returns !carry flag)
  int operator--(int); ///< subtraction operator (returns !carry flag)
  int operator--();    ///< subtraction operator (returns !carry flag)

  int equivalent(const BnId &) const; ///< Equal but - is assume to be ok
  void reverse();                     ///< Swap -1 to 1 adn leaver the zeros

  int PIstatus() const { return PI; } ///< PI accessor
  void setPI(const int A) { PI = A; } ///< PI accessor
  int intValue() const;               ///< Integer from binary expression
  std::pair<int, BnId> makeCombination(const BnId &) const;

  /// Total requiring expression
  size_t expressCount() const { return size - Znum; }
  /// returns number of variables / size
  size_t Size() const { return size; }
  /// Access true count
  int TrueCount() const { return Tnum; }

  void mapState(const std::vector<int> &, std::map<int, int> &) const;

  /// Displays the value as a string
  std::string display() const;
  /// writes the value to a stream
  void write(std::ostream &) const;
};

std::ostream &operator<<(std::ostream &, const BnId &);

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
