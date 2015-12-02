#include "MantidCurveFitting/Functions/Chebfun.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/optional.hpp>
#include <list>
#include <limits>
#include <iostream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

namespace {

size_t g_maxSplitLevel = 100;

//------------------------------------------------------------------------------
/// Split the fitting interval in half recursively until for each sub-interval
/// the function can be approximated to the required accuracy.
std::list<SimpleChebfun> splitFit(ChebfunFunctionType fun, double start,
                                  double end, double accuracy, size_t badSize, size_t &level) {

  std::list<SimpleChebfun> chebs;
  double middle = (start + end) / 2;

  std::vector<double> p, a;
  auto base = ChebfunBase::bestFit(start, middle, fun, p, a, 0.0, accuracy);

  if (base)
  {
    SimpleChebfun leftFun(base);
    leftFun.setData(p, a);
    chebs.insert(chebs.end(), leftFun);
  }
  else
  {
    ++level;
    if (level > g_maxSplitLevel)
    {
      return std::list<SimpleChebfun>();
    }
    std::list<SimpleChebfun> leftChebs = splitFit(fun, start, middle, accuracy, badSize, level);
    chebs.insert(chebs.end(), leftChebs.begin(), leftChebs.end());
  }

  base = ChebfunBase::bestFit(middle, end, fun, p, a, 0.0, accuracy);
  if (base)
  {
    SimpleChebfun rightFun(base);
    rightFun.setData(p, a);
    chebs.insert(chebs.end(), rightFun);
  }
  else
  {
    ++level;
    if (level > g_maxSplitLevel)
    {
      return std::list<SimpleChebfun>();
    }
    std::list<SimpleChebfun> rightChebs = splitFit(fun, middle, end, accuracy, badSize, level);
    chebs.insert(chebs.end(), rightChebs.begin(), rightChebs.end());
  }
  return chebs;

}

//------------------------------------------------------------------------------
/// Try to find the best approximation to the required accuracy.
/// If failed return an empty list.
std::list<SimpleChebfun> bestFit(ChebfunFunctionType fun, double start,
                                 double end, double accuracy, size_t badSize,
                                 size_t &level) {
  std::vector<double> p, a;
  auto base = ChebfunBase::bestFit(start, end, fun, p, a, 0.0, accuracy);
  if (!base) {
    auto split = splitFit(fun, start, end, accuracy, badSize, level);
    //if (level >= g_maxSplitLevel) throw std::runtime_error("Failed to build a Chebfun.");
    return split;
  }
  SimpleChebfun cheb(base);
  cheb.setData(p, a);
  return std::list<SimpleChebfun>(1, cheb);
}

//------------------------------------------------------------------------------
/// Try to join two SimpleChebfuns into one keeping the highest accuracy.
/// Must be true: fun1.endX() == fun2.startX()
boost::optional<SimpleChebfun> join(const SimpleChebfun &fun1, const SimpleChebfun &fun2) {
  if (fun1.size() + fun2.size() >= ChebfunBase::maximumSize()) {
    return boost::optional<SimpleChebfun>();
  }
  const double sizeRatio = static_cast<double>(fun1.size()) / static_cast<double>(fun2.size());
  if (sizeRatio > 10.0 || sizeRatio < 0.1) {
    return boost::optional<SimpleChebfun>();
  }
  double border = fun1.endX();
  assert(fun2.startX() == border);
  auto accuracy = std::min(fun1.accuracy(), fun2.accuracy());
  auto fun = [&fun1, &fun2, &border](double x) { return x <= border ? fun1(x) : fun2(x); };
  std::vector<double> p, a;
  auto base = ChebfunBase::bestFit(fun1.startX(), fun2.endX(), fun, p, a, 0.0, accuracy);
  if (base && base->size() < 10*(fun1.size() + fun2.size())) {
    SimpleChebfun cheb(base);
    cheb.setData(p, a);
    return cheb;
  }
  return boost::optional<SimpleChebfun>();
}

//------------------------------------------------------------------------------
/// Try to join as many SimpleChebfuns as possible.
std::list<SimpleChebfun> join(const std::list<SimpleChebfun> &parts) {
  std::list<SimpleChebfun> chebs;
  auto part2 = parts.begin();
  chebs.push_back(*part2);
  ++part2;
  for (; part2 != parts.end(); ++part2) {
    auto &part1 = chebs.back();
    if (!part2->isGood() || !part1.isGood()) {
      chebs.push_back(*part2);
      continue;
    }
    auto joined = join(part1, *part2);
    if (joined) {
      chebs.pop_back();
      chebs.push_back(joined.get());
    } else {
      chebs.push_back(*part2);
    }
  }
  return chebs.size() < parts.size() ? chebs : std::list<SimpleChebfun>();
}

}

//------------------------------------------------------------------------------
/// Constructor
Chebfun::Chebfun(ChebfunFunctionType fun, double start, double end,
                 double accuracy, size_t badSize) {
  bestFitAnyAccuracy(fun, start, end, accuracy, badSize);
}

//------------------------------------------------------------------------------
/// Number of smooth parts
void Chebfun::bestFitAnyAccuracy(ChebfunFunctionType fun, double start,
                                 double end, double accuracy, size_t badSize) {

  if (accuracy == 0.0)  {
    accuracy = ChebfunBase::defaultTolerance();
  }

  for (double acc = accuracy; acc < 0.1; acc *= 100) {
    size_t level = 0;
    auto chebs = bestFit(fun, start, end, acc, badSize, level);
    if (!chebs.empty()) {
      auto joinedChebs = join(chebs);
      if (joinedChebs.empty()) {
        m_parts.assign(chebs.begin(), chebs.end());
      } else {
        m_parts.assign(joinedChebs.begin(), joinedChebs.end());
      }
      break;
    }
  }
  if (m_parts.empty()) {
    throw std::runtime_error("Failed to create a Chebfun.");
  }
  m_startX = m_parts.front().startX();
  m_endX = m_parts.back().endX();
}

//------------------------------------------------------------------------------
/// Number of smooth parts
size_t Chebfun::numberOfParts() const {
  return m_parts.size();
}

//------------------------------------------------------------------------------
/// Evaluate the function.
double Chebfun::operator()(double x) const {
  for (auto &part : m_parts) {
    if (x >= part.startX() && x <= part.endX()) {
      return part(x);
    }
  }
  return 0.0;
}

//------------------------------------------------------------------------------
/// Evaluate the function.
std::vector<double> Chebfun::operator()(const std::vector<double> &x) const {
  return std::vector<double>();
}

//------------------------------------------------------------------------------
/// Total size of the approximation
size_t Chebfun::size() const {
  size_t totalSize = 0;
  for (auto &p : m_parts) {
    totalSize += p.size();
  }
  return totalSize;
}

//------------------------------------------------------------------------------
/// Get the worst accuracy
double Chebfun::accuracy() const {
  double worst = 0.0;
  for (auto &p : m_parts) {
    if (p.accuracy() > worst) {
      worst = p.accuracy();
    }
  }
  return worst;
}

//------------------------------------------------------------------------------
/// Is approximation good?
bool Chebfun::isGood() const {
  for (auto &p : m_parts) {
    if (!p.isGood()) {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
/// Get all break points
std::vector<double> Chebfun::getBreakPoints() const {
  std::vector<double> breaks;
  breaks.reserve(m_parts.size() + 1);
  for (auto &p : m_parts) {
    breaks.push_back(p.startX());
  }
  breaks.push_back(endX());
  return breaks;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
