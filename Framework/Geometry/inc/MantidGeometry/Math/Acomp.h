// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Math/BnId.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Mantid {
namespace Kernel {
template <typename T> class Matrix;
}

namespace Geometry {

/**
\class Acomp
\brief Holds a state point in the decision tree.
\version 1.0
\author S. Ansell
\date April 2005

Holds the state of a logical unit. Either a
state point and intersection or a unit
A component, either a union or intersection.
It has to have the ability to change the type
from union/intersection in an instant. Hence the type flag.
This currently works with forming disjunction form.
The disjunction form is "or" [ v symbol ]. Thus is is
a set of unions.
- intersection : (logical and) : ^ : e.g abcd
- union : (logical or) : v : e.g a+b+c+d

*/

class MANTID_GEOMETRY_DLL Acomp {

private:
  int Intersect;           ///<  Union/Intersection (0,1)
  std::vector<int> Units;  ///< Units in list
  std::vector<Acomp> Comp; ///< Components in list

  void deleteComp();           ///< delete all of the Comp list
  void addComp(const Acomp &); ///< add a Component intelligently
  void addUnitItem(int const); ///< add an Unit intellgently
  void processIntersection(const std::string &);
  void processUnion(const std::string &);
  int joinDepth();                ///< Search table to uplift objects
  int removeEqComp();             ///< Remove non-unique items
  int copySimilar(const Acomp &); ///< Join two componenet of similar type

  void addUnit(const std::vector<int> &,
               const BnId &); ///< Adds a Binary state to the Component
  void assignDNF(const std::vector<int> &,
                 const std::vector<BnId> &); ///< Assigns the Comp with the DNF
  int getDNFobject(std::vector<int> &, std::vector<BnId> &) const;
  int getDNFpart(std::vector<Acomp> &) const; ///< get the DNF parts (as Acomp)

  int getCNFobject(std::vector<int> &, std::vector<BnId> &) const;
  void assignCNF(const std::vector<int> &,
                 const std::vector<BnId> &); ///< Assigns the Comp with the DNF
  //  int getCNFpart(std::vector<Acomp>&) const;                   ///< get the
  //  CNF parts (as Acomp)

  /// Calculate Principal Components
  int makePI(std::vector<BnId> &) const;
  int makeEPI(std::vector<BnId> &, std::vector<BnId> &) const;

  int makeReadOnce(); ///< Factorize into a read once function

public:
  Acomp(int const = 0);
  bool operator==(const Acomp &) const;
  bool operator!=(const Acomp &) const; ///< Complementary operator
  bool operator<(const Acomp &) const;
  bool operator>(const Acomp &) const;
  Acomp &operator+=(const Acomp &);
  Acomp &operator-=(const Acomp &);
  Acomp &operator*=(const Acomp &);

  const Acomp *itemC(int const) const; ///< returns a pointer to Comp (or zero)
  int itemN(int const) const;          ///< returns an integer to Units (or zero)

  std::pair<int, int> size() const; ///< get the size of the units and the Acomp sub-comp
  int isSimple() const;             ///< true if only Units
  int isDNF() const;                ///< is Units only in union
  int isCNF() const;                ///< is Units only in intersections
  int isNull() const;               ///< is nothing in the comp.
  int isSingle() const;             ///< only one part
  int contains(const Acomp &) const;
  int isInter() const { return Intersect; }     ///< Deterimine if inter/union
  int isTrue(const std::map<int, int> &) const; ///< Determine if the rule is true.

  void Sort();                                     ///< Sort the Units+Comp items
  void getLiterals(std::map<int, int> &) const;    ///< Get literals (+/- different)
  void getAbsLiterals(std::map<int, int> &) const; ///< Get literals (positve)
  std::vector<int> getKeys() const;
  int logicalEqual(const Acomp &) const;

  int makeDNFobject(); ///< Make the object into DNF form (Sum of Products)
  int makeCNFobject(); ///< Make the object into CNF form (Product of Sums)

  void complement();                             ///< Take complement of component
  std::pair<Acomp, Acomp> algDiv(const Acomp &); ///< Carry out Algebric division
  void setString(const std::string &);           ///< Processes a line of type abc'+efg
  void writeFull(std::ostream &,
                 int const = 0) const;           ///< Full write out to determine state
  std::string display() const;                   ///< Pretty print statment
  std::string displayDepth(int const = 0) const; ///< Really pretty print statment of tree

  void printImplicates(const std::vector<BnId> &, const Kernel::Matrix<int> &) const;
};

std::ostream &operator<<(std::ostream &, const Acomp &);

} // NAMESPACE Geometry

} // NAMESPACE Mantid
