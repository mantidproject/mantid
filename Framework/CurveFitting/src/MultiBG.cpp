//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/MultiBG.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/FunctionFactory.h"

#include <boost/lambda/lambda.hpp>
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <float.h>

using namespace boost::lambda;

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(MultiBG)

/** A Jacobian for individual functions
 */
class PartialJacobian : public API::Jacobian {
  API::Jacobian *m_J; ///< pointer to the overall Jacobian
  size_t m_iY0; ///< data array offset in the overall Jacobian for a particular
  /// function
  size_t m_iP0; ///< parameter offset in the overall Jacobian for a particular
  /// function
  size_t m_iaP0; ///< offset in the active Jacobian for a particular function
public:
  /** Constructor
   * @param J :: A pointer to the overall Jacobian
   * @param iY0 :: Data array offset index (declared) for a particular function
   * @param iP0 :: The parameter index (declared) offset for a particular
   * function
   * @param iap0 :: The active parameter index (declared) offset for a
   * particular function
   */
  PartialJacobian(API::Jacobian *J, size_t iY0, size_t iP0, size_t iap0)
      : m_J(J), m_iY0(iY0), m_iP0(iP0), m_iaP0(iap0) {}
  /**
   * Overridden Jacobian::set(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   * @param value :: The derivative value
   */
  void set(size_t iY, size_t iP, double value) {
    m_J->set(m_iY0 + iY, m_iP0 + iP, value);
  }
  /**
   * Overridden Jacobian::get(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   */
  double get(size_t iY, size_t iP) { return m_J->get(m_iY0 + iY, m_iP0 + iP); }
  /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
   *   @param value :: Value to add
   *   @param iActiveP :: The index of an active parameter.
   */
  virtual void addNumberToColumn(const double &value, const size_t &iActiveP) {
    m_J->addNumberToColumn(value, m_iaP0 + iActiveP);
  }
};

/// Destructor
MultiBG::~MultiBG() {}

/**
 * Function you want to fit to.
 */
void MultiBG::function(double *out) const {
  std::vector<double> tmpOut(dataSize());
  std::fill_n(out, dataSize(), 0);
  for (size_t i = 0; i < nFunctions(); i++) {
    IFitFunction *fun = dynamic_cast<IFitFunction *>(getFunction(i));
    if (!fun) {
      throw std::runtime_error(
          "IFitFunction expected but function of another type found");
    }
    size_t nWS = m_funIndex[i].size();
    for (size_t k = 0; k < nWS; ++k) {
      size_t j = m_funIndex[i][k];
      fun->setWorkspace(m_spectra[k].first,
                        "WorkspaceIndex=" + boost::lexical_cast<std::string>(
                                                m_spectra[j].second),
                        false);
      // std::cerr << i << ' ' << k << " Function " << fun->name() << " ws " <<
      // fun->getWorkspace()->getName() << " wi "
      //  << dynamic_cast<Mantid::API::IFunctionMW*>(fun)->getWorkspaceIndex()
      //  << std::endl;
      double *out1 = out + m_offset[j];
      double *tmp1 = &tmpOut[0] + m_offset[j];
      size_t nData = 0;
      if (j < m_offset.size() - 1)
        nData = m_offset[j + 1] - m_offset[j];
      else
        nData = dataSize() - m_offset[j];
      if (i == 0) {
        fun->function(out1);
      } else {
        fun->function(tmp1);
        std::transform(out1, out1 + nData, tmp1, out1, std::plus<double>());
      }
    }
  }
  // std::cerr << "Function:\n";
  // for(size_t i = 0; i<nParams();++i)
  //{
  //  std::cerr << getParameter(i) << ' ' ;
  //}
  // std::cerr << std::endl;
  // std::for_each(out,out+m_dataSize,std::cerr << _1 << '\n');
  // std::cerr << std::endl;
}

void MultiBG::functionDeriv(API::Jacobian *out) {
  // it is possible that out is NULL
  if (!out)
    return;
  for (size_t i = 0; i < nFunctions(); i++) {
    IFitFunction *fun = dynamic_cast<IFitFunction *>(getFunction(i));
    if (!fun) {
      throw std::runtime_error(
          "IFitFunction expected but function of another type found");
    }
    size_t nWS = m_funIndex[i].size();
    for (size_t k = 0; k < nWS; ++k) {
      size_t j = m_funIndex[i][k];
      fun->setWorkspace(m_spectra[k].first,
                        "WorkspaceIndex=" + boost::lexical_cast<std::string>(
                                                m_spectra[j].second),
                        false);
      PartialJacobian J(out, m_offset[j], paramOffset(i), activeOffset(i));
      fun->functionDeriv(&J);
    }
  }
}

void MultiBG::setWorkspace(boost::shared_ptr<const API::Workspace> ws, bool) {
  boost::shared_ptr<const API::MatrixWorkspace> mws =
      boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws);
  if (ws && !mws) {
    throw std::invalid_argument(
        "Workspace in MultiBG has a wrong type (not a MatrixWorkspace)");
  }
  m_workspace = mws;
}
/**
 * Sets workspaces to member functions. Constructs the data set for fitting.
 * @param slicing :: A map between member functions and workspaces or empty
 * string. Format:
 *   "f0,Workspace0,i0;f1,Workspace1,i1;f2,Workspace2,i2;..."
 */
void MultiBG::setSlicing(const std::string &slicing) {
  boost::shared_ptr<const API::MatrixWorkspace> mws = m_workspace;

  m_funIndex.resize(nFunctions());

  if (!slicing.empty()) {
    Mantid::API::Expression expr;
    expr.parse(slicing);
    // expr can be treated as a list even if it has only 1 term
    expr.toList(";");
    for (size_t i = 0; i < expr.size(); ++i) {
      const Mantid::API::Expression &e = expr[i];
      if (e.name() != "," || e.size() != 3) {
        // slicing has a wrong format - ignore it
        break;
      }
      try {
        std::string wsName = e[1].name();
        Mantid::API::MatrixWorkspace_sptr ws =
            boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                Mantid::API::AnalysisDataService::Instance().retrieve(wsName));

        size_t iFun = boost::lexical_cast<size_t>(e[0].name().substr(1));
        size_t wi = boost::lexical_cast<size_t>(e[2].name());
        if (iFun >= nFunctions()) {
          throw std::invalid_argument("MultiBG::setWorkspace: function " +
                                      e[0].name() + " not found");
        }
        std::pair<boost::shared_ptr<const API::MatrixWorkspace>, size_t>
            spectrum = std::make_pair(ws, wi);
        m_funIndex[iFun].push_back(m_spectra.size());
        m_spectra.push_back(spectrum);
        IFitFunction *fun = dynamic_cast<IFitFunction *>(getFunction(iFun));
        if (!fun) {
          throw std::runtime_error(
              "IFitFunction expected but function of another type found");
        }
        fun->setWorkspace(ws, "WorkspaceIndex=" + e[2].name(), false);
      } catch (...) {
        break;
      }
    }
  }

  // examine the member functions and fill in the m_funIndex array
  for (size_t iFun = 0; iFun < nFunctions(); iFun++) {
    API::IFunctionMW *fun = dynamic_cast<API::IFunctionMW *>(getFunction(iFun));
    if (!fun) {
      throw std::runtime_error("MultiBG works with IFunctionMW only");
    }
    if (fun->getWorkspace()) {
      boost::shared_ptr<const API::MatrixWorkspace> iws =
          fun->getMatrixWorkspace();
      std::pair<boost::shared_ptr<const API::MatrixWorkspace>, size_t>
          spectrum = std::make_pair(iws, fun->getWorkspaceIndex());
      std::vector<std::pair<boost::shared_ptr<const API::MatrixWorkspace>,
                            size_t>>::iterator it =
          std::find(m_spectra.begin(), m_spectra.end(), spectrum);
      size_t i;
      if (it == m_spectra.end()) {
        i = m_spectra.size();
        m_spectra.push_back(spectrum);
      } else {
        i = size_t(std::distance(it, m_spectra.begin()));
      }
      m_funIndex[iFun].push_back(i);
      // fun->setWorkspace(boost::static_pointer_cast<const
      // API::Workspace>(iws),slicing,false);
    }
  }

  // setWorkspace can be called by GUI when the function had not been properly
  // initialized
  if (m_spectra.empty()) {
    return;
  }

  // make functions without set workspace fit to all workspaces
  for (size_t iFun = 0; iFun < nFunctions(); iFun++) {
    std::vector<size_t> &index = m_funIndex[iFun];
    if (index.empty()) {
      index.resize(m_spectra.size());
      int i = 0;
      std::for_each(index.begin(), index.end(), _1 = var(i)++);
      IFitFunction *fun = dynamic_cast<IFitFunction *>(getFunction(iFun));
      if (!fun) {
        throw std::runtime_error(
            "IFitFunction expected but function of another type found");
      }
      fun->setWorkspace(m_spectra[0].first,
                        "WorkspaceIndex=" + boost::lexical_cast<std::string>(
                                                m_spectra[0].second),
                        false);
    }
  }

  // set dimensions and calculate ws's contribution to m_dataSize
  // IFunctionMW::setWorkspace(ws,slicing,false);
  // add other workspaces
  m_offset.resize(m_spectra.size(), 0);
  size_t nData = 0;
  for (size_t i = 0; i < m_spectra.size(); ++i) {
    mws = m_spectra[i].first;
    size_t n = mws->blocksize();
    m_offset[i] = nData;
    nData += static_cast<int>(n);
  }

  m_data.resize(nData);
  m_weights.resize(nData);

  //... fill in the data and the weights ...

  for (size_t i = 0; i < m_spectra.size(); ++i) {
    mws = m_spectra[i].first;
    size_t wi = m_spectra[i].second;
    const Mantid::MantidVec &Y = mws->readY(wi);
    const Mantid::MantidVec &E = mws->readE(wi);
    size_t j0 = m_offset[i];
    for (size_t j = 0; j < Y.size(); ++j) {
      m_data[j0 + j] = Y[j];
      double err = E[j];
      m_weights[j0 + j] = err != 0.0 ? 1. / err : 1.0;
    }
  }

  // std::cerr << "Workspace:\n";
  // std::for_each(&m_data[0],&m_data[0]+m_dataSize,std::cerr << _1 << '\n');
}

/**
 * Creates a workspace containing values calculated with this function. It takes
 * a workspace and ws index
 * of a spectrum which this function may have been fitted to. The output
 * contains the original spectrum
 * (wi = 0), the calculated values (ws = 1), and the difference between them (ws
 * = 2).
 * @param sd :: optional standard deviations of the parameters for calculating
 * the error bars
 * @return created workspase
 */
boost::shared_ptr<API::WorkspaceGroup>
MultiBG::createCalculatedWorkspaceGroup(const std::vector<double> &sd) {
  UNUSED_ARG(sd)
  return boost::shared_ptr<API::WorkspaceGroup>();
}
} // namespace API
} // namespace Mantid
