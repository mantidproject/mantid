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
std::list<SimpleChebfun> splitFit(size_t &level, ChebfunFunctionType fun, double start,
                                  double end, const Chebfun::Options &options) {

  std::list<SimpleChebfun> chebs;
  double middle = (start + end) / 2;

  std::vector<double> p, a;
  auto base = ChebfunBase::bestFit(start, middle, fun, p, a, options.accuracy, options.maxPartSize);

  if (base) {
    SimpleChebfun leftFun(base);
    leftFun.setData(p, a);
    chebs.insert(chebs.end(), leftFun);
  } else {
    ++level;
    if (level > options.maxParts) {
      if (options.doNotFail) {
        chebs.push_back(SimpleChebfun(options.badPartSize, fun, start, middle));
      } else {
        return std::list<SimpleChebfun>();
      }
    } else {
      std::list<SimpleChebfun> leftChebs =
          splitFit(level, fun, start, middle, options);
      chebs.insert(chebs.end(), leftChebs.begin(), leftChebs.end());
    }
  }

  base = ChebfunBase::bestFit(middle, end, fun, p, a, options.accuracy, options.maxPartSize);
  if (base) {
    SimpleChebfun rightFun(base);
    rightFun.setData(p, a);
    chebs.insert(chebs.end(), rightFun);
  } else {
    ++level;
    if (level > options.maxParts) {
      if (options.doNotFail) {
        chebs.push_back(SimpleChebfun(options.badPartSize, fun, middle, end));
      } else {
        return std::list<SimpleChebfun>();
      }
    } else {
      std::list<SimpleChebfun> rightChebs =
          splitFit(level, fun, middle, end, options);
      chebs.insert(chebs.end(), rightChebs.begin(), rightChebs.end());
    }
  }
  return chebs;
}


//------------------------------------------------------------------------------
/// Try to find the best approximation to the required accuracy.
/// If failed return an empty list.
std::list<SimpleChebfun> bestFit(size_t &level, ChebfunFunctionType fun,
                                 double start, double end, const Chebfun::Options &options) {
  std::vector<double> p, a;
  auto base = ChebfunBase::bestFit(start, end, fun, p, a, options.accuracy, options.maxPartSize);
  if (!base) {
    auto split = splitFit(level, fun, start, end, options);
    if (!options.doNotFail && level >= options.maxParts) throw std::runtime_error("Failed to build a Chebfun.");
    return split;
  }
  SimpleChebfun cheb(base);
  cheb.setData(p, a);
  return std::list<SimpleChebfun>(1, cheb);
}

//------------------------------------------------------------------------------
/// Try to join two SimpleChebfuns into one keeping the highest accuracy.
/// Must be true: fun1.endX() == fun2.startX()
boost::optional<SimpleChebfun> join(const SimpleChebfun &fun1, const SimpleChebfun &fun2, const Chebfun::Options &options) {
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
  auto base = ChebfunBase::bestFit(fun1.startX(), fun2.endX(), fun, p, a, accuracy, options.maxPartSize);
  if (base && base->size() < 10*(fun1.size() + fun2.size())) {
    SimpleChebfun cheb(base);
    cheb.setData(p, a);
    return cheb;
  }
  return boost::optional<SimpleChebfun>();
}

//------------------------------------------------------------------------------
/// Try to join as many SimpleChebfuns as possible.
std::list<SimpleChebfun> join(const std::list<SimpleChebfun> &parts, const Chebfun::Options &options) {
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
    auto joined = join(part1, *part2, options);
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
/// Options constructor
Chebfun::Options::Options(double acc, size_t mp, size_t mps, bool dnf,
                          size_t bps)
    : accuracy(acc), maxParts(mp), maxPartSize(mps), doNotFail(dnf),
      badPartSize(bps) {
  if (accuracy == 0.0) {
    accuracy = ChebfunBase::defaultTolerance();
  }

  if (maxParts == 0) {
    maxParts = g_maxSplitLevel;
  }

  if (maxPartSize == 0) {
    maxPartSize = ChebfunBase::maximumSize();
  }
}

//------------------------------------------------------------------------------
/// Constructor
Chebfun::Chebfun(ChebfunFunctionType fun, double start, double end,
                 double accuracy) {
  Options options(accuracy);
  bestFitAnyAccuracy(fun, start, end, options);
}

//------------------------------------------------------------------------------
/// Constructor
Chebfun::Chebfun(ChebfunFunctionType fun, double start, double end,
                 const Chebfun::Options &options) {
  bestFitAnyAccuracy(fun, start, end, options);
}

//------------------------------------------------------------------------------
/// Constructor
Chebfun::Chebfun(std::vector<SimpleChebfun>&& parts):m_parts(parts){
  m_startX = parts.front().startX();
  m_endX = parts.back().endX();
}

//------------------------------------------------------------------------------
/// Number of smooth parts
void Chebfun::bestFitAnyAccuracy(ChebfunFunctionType fun, double start,
                                 double end, const Chebfun::Options &options) {

  Options opts = options;
  for (double acc = options.accuracy; acc < 0.1; acc *= 100) {
    size_t level = 0;
    opts.accuracy = acc;
    auto chebs = bestFit(level, fun, start, end, options);
    if (!chebs.empty()) {
      auto joinedChebs = join(chebs, options);
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
/// Get the width of the interval
double Chebfun::width() const{
  return m_endX - m_startX;
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
/// Evaluate the function for all values in a vector. TODO: more efficient implementation.
/// @param x :: A vector with arguments.
std::vector<double> Chebfun::operator()(const std::vector<double> &x) const {
  std::vector<double> res;
  res.reserve(x.size());
  for(auto xi : x) {
    res.push_back((*this)(xi));
  }
  return res;
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

//------------------------------------------------------------------------------
/// Get all x - points
std::vector<double> Chebfun::getAllXPoints() const {
  std::vector<double> res;
  for (auto &p : m_parts) {
    auto &x = p.xPoints();
    res.insert(res.end(), x.begin(), x.end());
  }
  return res;
}

//------------------------------------------------------------------------------
/// Get all y - points
std::vector<double> Chebfun::getAllYPoints() const {
  std::vector<double> res;
  for (auto &p : m_parts) {
    auto &y = p.yPoints();
    res.insert(res.end(), y.begin(), y.end());
  }
  return res;
}

//------------------------------------------------------------------------------
/// Create a derivative of this function.
Chebfun Chebfun::derivative() const{
  std::vector<SimpleChebfun> derivativeParts;
  derivativeParts.reserve(m_parts.size());
  for (auto &part : m_parts) {
    derivativeParts.push_back(part.derivative());
  }
  return Chebfun(std::move(derivativeParts));
}

//------------------------------------------------------------------------------
/// Get rough estimates of the roots
/// @param level :: An optional right-hand-side of equation (*this)(x) == level.
std::vector<double> Chebfun::roughRoots(double level) const{
  std::vector<double> roots;
  for(auto &part : m_parts) {
    auto partRoots = part.roughRoots(level);
    roots.insert(roots.end(), partRoots.begin(), partRoots.end());
  }
  return roots;
}

/// Create a vector of x values linearly spaced on the approximation interval
std::vector<double> Chebfun::linspace(size_t n) const {
  return ChebfunBase::linspace(n, m_startX, m_endX);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
