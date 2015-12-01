#include "MantidCurveFitting/Functions/Chebfun.h"

#include <list>
#include <iostream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

namespace {

size_t g_maxSplitLevel = 10;

//------------------------------------------------------------------------------
std::list<SimpleChebfun> splitFit(ChebfunFunctionType fun, double start,
                                  double end, double accuracy, size_t badSize, size_t &level) {

  std::list<SimpleChebfun> chebs;
  double middle = (start + end) / 2;

  SimpleChebfun leftFun(fun, start, middle, accuracy, badSize);

  std::vector<double> p, a;
  auto base = ChebfunBase::bestFit(start, end, fun, p, a, 0.0, accuracy);

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

  base = ChebfunBase::bestFit(start, end, fun, p, a, 0.0, accuracy);
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
    std::vector<double> p, a;
    auto base = ChebfunBase::bestFit(start, end, fun, p, a, 0.0, acc);
    if (!base) {
      size_t level = 0;
      auto chebs = splitFit(fun, start, end, acc, badSize, level);
      if (!chebs.empty())
      {
        m_parts.assign(chebs.begin(), chebs.end());
        break;
      }
    }
    else {
      SimpleChebfun first(base);
      first.setData(p, a);
      m_parts.push_back(first);
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


} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
