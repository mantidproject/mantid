// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

#include "MantidGeometry/Math/BnId.h"

namespace Mantid {

namespace Geometry {

/** Output as a stream
 *  @param of :: Reference to the output stream
 *  @param A ::  The object to output
 *  @return stream representation
 */
std::ostream &operator<<(std::ostream &of, const BnId &A) {
  of << A.display();
  return of;
}

BnId::BnId()
    : size(0), PI(1), Tnum(0), Znum(0)
/**
  Standard Constructor
*/
{}

BnId::BnId(const size_t A, const unsigned int X)
    : size(A), PI(1), Tnum(0), Znum(0), Tval(A)
/**
  Constructer that creates a true/false mapping
  without the  undetermined option
  @param A :: size of the vector Tval. (number of surfaces)
  @param X :: interger for of the binary representation
*/
{
  unsigned int cnt = 1;
  int sum(0);
  for (size_t i = 0; i < size; cnt *= 2, i++) {
    Tval[i] = (X & cnt) ? 1 : -1;
    sum += Tval[i];
  }
  Tnum = (sum + static_cast<int>(size)) / 2;
}

bool BnId::operator==(const BnId &A) const
/**
  Tri-state return of the equality
  @param A :: BnId object to compare
  @retval 1 == absolutely identical
  @retval 0 == not equal
*/
{
  if (A.size != size || A.Tnum != Tnum || A.Znum != Znum)
    return false;
  auto ac = A.Tval.cbegin();
  for (auto vc = Tval.cbegin(); vc != Tval.cend(); ++vc, ++ac) {
    if (ac == A.Tval.cend()) // This should never happen
      return false;
    if (*vc != *ac)
      return false;
  }
  return true;
}

int BnId::equivalent(const BnId &A) const
/**
  Tri-state return of the equality
  @param A :: BnId object to compare
  @retval 1 == absolutely identical
  @retval 0 == not equal
*/
{
  if (A.size != size)
    return 0;
  int retval = 1;
  for (size_t i = 0; i < size; i++) {
    if (Tval[i] * A.Tval[i] < 0) // true * false == -1.
      return 0;
    if (retval == 1 && Tval[i] != Tval[i])
      retval = 2;
  }
  return retval;
}

bool BnId::operator>(const BnId &A) const
/**
  Tri-state return of the ordering of number of true
  @param A :: BnId object to compare
  @return !(this<A)
*/
{
  return (&A != this) ? !(*this < A) : false;
}

bool BnId::operator<(const BnId &A) const
/**
  Tri-state return of the ordering of number of true states
  @param A :: BnId object to compare
  @return Size<A.size, N of True<A.N of True,
     IntVal<A.IntVal
*/
{
  if (A.size != size)
    return size < A.size;
  if (Znum != A.Znum)
    return (Znum < A.Znum);

  if (Tnum != A.Tnum)
    return (Tnum < A.Tnum);

  auto tvc = Tval.crbegin();
  auto avc = A.Tval.crbegin();
  while (tvc != Tval.crend()) {
    if (*tvc != *avc)
      return *tvc < *avc;
    ++tvc;
    ++avc;
  }
  return false;
}

int BnId::operator[](const int A) const
/**
  Returns the particular rule value
  @param A :: array offset 0->size-1
  @return -99 on err, Tval[A] normally
*/
{
  if (A < 0 && A >= static_cast<int>(size))
    return -99;
  return Tval[A];
}

int BnId::operator++(int)
/**
  convertion to ++operator (prefix)
  from operator++ (postfix)
  @retval 0 :: the function has looped (carry flag)
  @retval 1 :: no loop occored
 */
{
  return this->operator++();
}

int BnId::operator++()
/**
  Addition operator
  Carrys out a rotational addition (effective binary
  addition. It ignores non-important flags (Tval[i]==0)
  @retval 0 :: the function has looped (carry flag)
  @retval 1 :: no loop occored
*/
{
  std::vector<int>::iterator vc;
  for (vc = Tval.begin(); vc != Tval.end() && (*vc) != -1; ++vc) {
    if (*vc == 1) {
      Tnum--;
      *vc = -1;
    }
  }
  if (vc == Tval.end())
    return 0;
  // Normal exit
  *vc = 1;
  Tnum++;
  return 1;
}

int BnId::operator--(const int)
/**
  Convertion to --operator (prefix)
  from operator-- (postfix)
  @retval 0 :: the function has looped (carry flag)
  @retval 1 :: no loop occored
 */
{
  return this->operator--();
}

int BnId::operator--()
/**
  Subtraction operator
  Carrys out a rotational subtraction (effective binary
  addition. It ignores non-important flags (Tval[i]==0)
  @retval 0 :: the function has looped (carry flag)
  @retval 1 :: no loop occored
*/
{
  std::vector<int>::iterator vc;
  for (vc = Tval.begin(); vc != Tval.end() && (*vc) != 1; ++vc)
    if (*vc == -1) {
      *vc = 1;
      Tnum++;
    }
  if (vc == Tval.end()) // Loop took place
    return 0;

  *vc = -1;
  Tnum--;
  return 1;
}

void BnId::setCounters()
/**
  Sets the counters Tnum and Znum
*/
{
  std::vector<int>::const_iterator vc;
  Tnum = 0;
  Znum = 0;
  for (vc = Tval.begin(); vc != Tval.end(); ++vc) {
    if (*vc == 1)
      Tnum++;
    else if (*vc == 0)
      Znum++;
  }
}

int BnId::intValue() const
/**
  Returns the lowest int value associated with
  the string
  @return lowest bit in the BnId vector
*/
{
  unsigned out(0);
  std::vector<int>::const_reverse_iterator vc;
  for (vc = Tval.rbegin(); vc != Tval.rend(); ++vc) {
    out <<= 1;
    out += ((*vc) == 1) ? 1 : 0;
  }
  return out;
}

void BnId::mapState(const std::vector<int> &Index,
                    std::map<int, int> &Base) const
/**
  Sets the components within base with true/false
  @param Index :: vector of Literal/Surface numbers
  @param Base :: map to be filled
*/
{
  std::vector<int>::const_iterator vc;
  int i(0);
  for (vc = Index.begin(); vc != Index.end(); ++vc, ++i)
    Base[*vc] = (Tval[i] == 1) ? 1 : 0;
}

std::pair<int, BnId> BnId::makeCombination(const BnId &A) const
/**
  Find if A and this can be differ by one
  1/-1 bit and make a 0 value for that bit
  @param A :: value to check
  Output
    -  0 :: nothing to do.
    -  1 :: complement found
    - -1 :: items are too differnt
  @return pair<status,BnId> of possible new PI
  BnId is valid only if status ==1
*/
{

  if (size != A.size) // sizes different
    return std::pair<int, BnId>(-1, BnId());

  // Zero unequal or 1 value to far apart
  if (Znum != A.Znum || (Tnum - A.Tnum) * (Tnum - A.Tnum) > 1)
    return std::pair<int, BnId>(-1, BnId());

  // no difference
  if (Tnum == A.Tnum)
    return std::pair<int, BnId>(0, BnId());

  int flag(0); // numb of diff
  auto avc = A.Tval.cbegin();
  std::vector<int>::const_iterator chpt; // change point
  for (auto tvc = Tval.cbegin(); tvc != Tval.cend(); ++tvc, ++avc) {
    if ((*avc * *tvc) < 0) // false/true
    {
      if (flag) // failed
        return std::pair<int, BnId>(0, BnId());
      ;
      flag = 1; // inc change counter
      chpt = tvc;
    } else if (*avc != *tvc) // failed on a 0 : value
      return std::pair<int, BnId>(0, BnId());
    ;
  }
  // Good value
  if (flag) {
    BnId PIout(*this);
    PIout.Tval[(chpt - Tval.begin())] = 0;
    PIout.setCounters();
    return std::pair<int, BnId>(1, PIout);
  }
  // Nothing to do.
  return std::pair<int, BnId>(0, BnId());
  ;
}

void BnId::reverse()
/**
  Reverset the bits.
  Transform 1 -> -1
*/
{
  transform(Tval.begin(), Tval.end(), Tval.begin(),
            std::bind2nd(std::multiplies<int>(), -1));
  setCounters();
}

std::string BnId::display() const
/**
  Simple display to string
  0 == false, - == any, 1 == true
  @return String value of true etc
 */
{
  std::string Out;
  std::vector<int>::const_reverse_iterator vc;
  std::ostringstream cx;
  for (vc = Tval.rbegin(); vc != Tval.rend(); ++vc) {
    if (*vc == 0)
      Out += "-";
    else if (*vc == 1)
      Out += "1";
    else
      Out += "0";
  }
  cx << "(" << Tnum << ":" << Znum << ")";
  return Out + cx.str();
}

void BnId::write(std::ostream &os) const { os << display(); }

} // NAMESPACE Geometry

} // NAMESPACE Mantid
