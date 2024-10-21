// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Jacobian.h"
#include "MantidKernel/Exception.h"

#include "MantidGeometry/muParser_Silent.h"
#include <boost/lambda/lambda.hpp>

#include <algorithm>
#include <cfloat>
#include <functional>
#include <iterator>
#include <sstream>

namespace Mantid::API {
using namespace Geometry;

/// Virtual copy constructor
std::shared_ptr<IFunction> IFunctionMD::clone() const {
  auto fun = IFunction::clone();
  return fun;
}

/** Set the workspace
 * @param ws :: A shared pointer to a workspace.
 */
void IFunctionMD::setWorkspace(std::shared_ptr<const Workspace> ws) {
  try {
    IMDWorkspace_const_sptr workspace = std::dynamic_pointer_cast<const IMDWorkspace>(ws);
    if (!workspace) {
      throw std::invalid_argument("Workspace has a wrong type (not a IMDWorkspace)");
    }

    if (m_dimensionIndexMap.empty()) {
      useAllDimensions(workspace);
    }

    m_dimensions.resize(m_dimensionIndexMap.size());
    std::map<std::string, size_t>::const_iterator it = m_dimensionIndexMap.begin();
    std::map<std::string, size_t>::const_iterator end = m_dimensionIndexMap.end();
    for (; it != end; ++it) {
      std::shared_ptr<const Mantid::Geometry::IMDDimension> dim = workspace->getDimensionWithId(it->first);
      if (!dim) {
        throw std::invalid_argument("Dimension " + it->first + " dos not exist in workspace " + ws->getName());
      }
      m_dimensions[it->second] = dim;
    }

  } catch (std::exception &) {
    throw;
  }
}

/**
 * @param domain :: The input domain over which to calculate the function
 * @param values :: A result holder to store the output of the calculation
 */
void IFunctionMD::function(const FunctionDomain &domain, FunctionValues &values) const {
  const auto *dmd = dynamic_cast<const FunctionDomainMD *>(&domain);
  if (!dmd) {
    throw std::invalid_argument("Unexpected domain in IFunctionMD");
  }
  evaluateFunction(*dmd, values);
}

/**
 * Performs the function evaluations on the domain
 *
 * @param domain :: The MD domain to evaluate the function
 * @param values :: The computed values
 */
void IFunctionMD::evaluateFunction(const FunctionDomainMD &domain, FunctionValues &values) const {
  domain.reset();
  size_t i = 0;
  for (const IMDIterator *r = domain.getNextIterator(); r != nullptr; r = domain.getNextIterator()) {
    this->reportProgress("Evaluating function for box " + std::to_string(i + 1));
    values.setCalculated(i, functionMD(*r));
    i++;
  };
}

/** User functions call this method in their constructors to set up the order of
 * the dimensions.
 * The dimensions will be sorted in the order of calls to useDimension.
 * Ordering is needed to
 * access dimensions by ineteger index rather than by name (string)
 * @param id :: The id of a dimension in the workspace
 */
void IFunctionMD::useDimension(const std::string &id) {
  size_t n = m_dimensionIndexMap.size();
  if (m_dimensionIndexMap.find(id) != m_dimensionIndexMap.end()) {
    throw std::invalid_argument("Dimension " + id + " has already been used.");
  }
  m_dimensionIndexMap[id] = n;
}

/**
 * This method is called if a function does not call to useDimension at all.
 * It adds all the dimensions in the workspace in the order they are in in that
 * workspace
 * then calls init().
 */
void IFunctionMD::useAllDimensions(IMDWorkspace_const_sptr workspace) {
  if (!workspace) {
    throw std::runtime_error("Method IFunctionMD::useAllDimensions() can only "
                             "be called after setting the workspace");
  }
  for (size_t i = 0; i < workspace->getNumDims(); ++i) {
    useDimension(workspace->getDimension(i)->getDimensionId());
  }
  this->initDimensions();
}

} // namespace Mantid::API

// #include "MantidAPI/ParamFunction.h"
// #include "MantidKernel/VMD.h"
//
//  using Mantid::Kernel::VMD;

// namespace Mantid
//{
//  namespace API
//  {
//
//    /**
//      * An example MD function. Gaussian in N dimensions
//      */
//    class GaussianMD: public IFunctionMD, public ParamFunction
//    {
//    public:
//      /**
//        * Defining gaussian's parameters here, ie after the workspace is set
//        and
//        * the dimensions are known.
//        */
//      void initDimensions()
//      {
//        if (!getWorkspace()) return;
//        std::map<std::string,size_t>::const_iterator it =
//        m_dimensionIndexMap.begin();
//        std::map<std::string,size_t>::const_iterator end =
//        m_dimensionIndexMap.end();
//        for(; it != end; ++it)
//        {
//          declareParameter(it->first+"_centre",0.0);
//          declareParameter(it->first+"_alpha",1.0);
//        }
//        declareParameter("Height",1.0);
//      }
//      std::string name() const {return "GaussianMD";}
//    protected:
//
//      /**
//        * Calculate the function value at a point r in the MD workspace
//        * @param r :: MD workspace iterator with a reference to the current
//        point
//        */
//      double functionMD(IMDIterator& r) const
//      {
//        double arg = 0.0;
//        size_t n = m_dimensions.size();
//        VMD center = r.getCenter();
//        for(size_t i = 0; i < n; ++i)
//        {
//          double c = getParameter(2*i);
//          double a = getParameter(2*i + 1);
//          double t = center[i] - c;
//          arg += a*t*t;
//        }
//        return getParameter("Height") * exp(-arg);
//      }
//    };
//
//    // Subscribe the function into the factory.
//    DECLARE_FUNCTION(GaussianMD);
//  } // API
//}   // Mantid
