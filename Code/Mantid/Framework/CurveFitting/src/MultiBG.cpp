//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/MultiBG.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/AnalysisDataService.h"

#include <boost/lambda/lambda.hpp>
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <float.h>

using namespace boost::lambda;

namespace Mantid
{
namespace CurveFitting
{

DECLARE_FUNCTION(MultiBG)

///Destructor
MultiBG::~MultiBG()
{
}

/** 
 * Function you want to fit to.
 */
void MultiBG::function(double* out)const
{
  std::vector<double> tmpOut(dataSize());
  std::fill_n(out,dataSize(),0);
  for(size_t i = 0; i < nFunctions(); i++)
  {
    IFitFunction* fun = getFunction(i);
    size_t nWS =  m_funIndex[i].size();
    for(size_t k = 0;k < nWS; ++k)
    {
      size_t j = m_funIndex[i][k];
      fun->setWorkspace(m_spectra[k].first,"WorkspaceIndex="+boost::lexical_cast<std::string>(m_spectra[j].second),false);
      //std::cerr << i << ' ' << k << " Function " << fun->name() << " ws " << fun->getWorkspace()->getName() << " wi "
      //  << dynamic_cast<Mantid::API::IFunctionMW*>(fun)->getWorkspaceIndex() << std::endl;
      double *out1 = out + m_offset[j];
      double *tmp1 = &tmpOut[0] + m_offset[j];
      size_t nData = 0;
      if (j < m_offset.size() - 1 )
        nData = m_offset[j + 1] - m_offset[j];
      else
        nData = dataSize() - m_offset[j];
      if (i == 0)
      {
        fun->function(out1);
      }
      else
      {
        fun->function(tmp1);
        std::transform(out1, out1 + nData, tmp1, out1, std::plus<double>());
      }
    }
  }
  //std::cerr << "Function:\n";
  //for(size_t i = 0; i<nParams();++i)
  //{
  //  std::cerr << getParameter(i) << ' ' ;
  //}
  //std::cerr << std::endl;
  //std::for_each(out,out+m_dataSize,std::cerr << _1 << '\n');
  //std::cerr << std::endl;
}

/// Derivatives of function with respect to active parameters
void MultiBG::functionDeriv(API::Jacobian* out)
{
    // it is possible that out is NULL
    if (!out) return;
    // claculate numerically
    double stepPercentage = DBL_EPSILON*1000; // step percentage
    double step; // real step
    const int nParam = nParams();
    const int nData  = dataSize();

    //for(size_t i=0; i < nParams(); ++i)
    //  std::cerr << i << ' ' << getParameter(i) << std::endl;
    //std::cerr << std::endl;

    std::vector<double> tmpFunctionOutputMinusStep(nData);
    std::vector<double> tmpFunctionOutputPlusStep(nData);

    function(&tmpFunctionOutputMinusStep[0]);

    for (int iP = 0; iP < nParam; iP++)
    {
      if ( isActive(iP) )
      {
        const double& val = getParameter(iP);
        if (fabs(val) < stepPercentage)
        {
          step = stepPercentage;
        }
        else
        {
          step = val*stepPercentage;
        }

        double paramPstep = val + step;
        setParameter(iP, paramPstep);
        function(&tmpFunctionOutputPlusStep[0]);

        step = paramPstep - val;
        setParameter(iP, val);

        for (int i = 0; i < nData; i++) 
        {
          double value = (tmpFunctionOutputPlusStep[i]-tmpFunctionOutputMinusStep[i])/step;
          out->set(i,iP,value);
        }
      }
    }

}

/**
 * Sets workspaces to member functions. Constructs the data set for fitting.
 * @param ws :: Pointer to a workspace, not used. Workspaces are taken either from member functions or slicing.
 * @param slicing :: A map between member functions and workspaces or empty string. Format:
 *   "f0,Workspace0,i0;f1,Workspace1,i1;f2,Workspace2,i2;..."
 */
void MultiBG::setWorkspace(boost::shared_ptr<const API::Workspace> ws,const std::string& slicing, bool)
{
  boost::shared_ptr<const API::MatrixWorkspace> mws = boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws);
  if (ws && !mws)
  {
    throw std::invalid_argument("Workspace in MultiBG has a wrong type (not a MatrixWorkspace)");
  }

  m_funIndex.resize(nFunctions());

  if (! slicing.empty())
  {
    Mantid::API::Expression expr;
    expr.parse(slicing);
    // expr can be treated as a list even if it has only 1 term
    expr.toList(";");
    for(size_t i=0;i<expr.size();++i)
    {
      const Mantid::API::Expression& e = expr[i];
      if (e.name() != "," || e.size() != 3)
      {
        // slicing has a wrong format - ignore it
        break;
      }
      try
      {
        std::string wsName = e[1].name();
        Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(wsName));


        size_t iFun = boost::lexical_cast<size_t>(e[0].name().substr(1));
        size_t wi = boost::lexical_cast<size_t>(e[2].name());
        if (iFun >= nFunctions())
        {
          throw std::invalid_argument("MultiBG::setWorkspace: function "+e[0].name()+" not found");
        }
        std::pair< boost::shared_ptr<const API::MatrixWorkspace>, size_t> spectrum = std::make_pair(ws,wi);
        m_funIndex[iFun].push_back(m_spectra.size());
        m_spectra.push_back(spectrum);
        getFunction(iFun)->setWorkspace(ws,"WorkspaceIndex="+e[2].name());
      }
      catch(...)
      {
        break;
      }
    }
  }

  // examine the member functions and fill in the m_funIndex array
  for(size_t iFun=0;iFun<nFunctions();iFun++)
  {
    API::IFunctionMW* fun = dynamic_cast<API::IFunctionMW*>(getFunction(iFun));
    if (! fun )
    {
      throw std::runtime_error("MultiBG works with IFunctionMW only");
    }
    if (fun->getWorkspace())
    {
      boost::shared_ptr<const API::MatrixWorkspace> iws =  fun->getMatrixWorkspace();
      std::pair< boost::shared_ptr<const API::MatrixWorkspace>, size_t> spectrum = std::make_pair(iws,fun->getWorkspaceIndex());
      std::vector< std::pair< boost::shared_ptr<const API::MatrixWorkspace>, size_t> >::iterator it = std::find(m_spectra.begin(),m_spectra.end(),spectrum);
      size_t i;
      if (it == m_spectra.end())
      {
        i = m_spectra.size();
        m_spectra.push_back(spectrum);
      }
      else
      {
        i = size_t(std::distance(it,m_spectra.begin()));
      }
      m_funIndex[iFun].push_back(i);
      //fun->setWorkspace(boost::static_pointer_cast<const API::Workspace>(iws),slicing,false);
    }
  }

  // make functions without set workspace fit to all workspaces
  for(size_t iFun=0;iFun<nFunctions();iFun++)
  {
    std::vector<size_t>& index = m_funIndex[iFun];
    if (index.empty())
    {
      index.resize(m_spectra.size());
      int i = 0;
      std::for_each(index.begin(),index.end(),_1 = var(i)++);
      getFunction(iFun)->setWorkspace(m_spectra[0].first,"WorkspaceIndex="+boost::lexical_cast<std::string>(m_spectra[0].second));
    }
  }

  // set dimensions and calculate ws's contribution to m_dataSize
  //IFunctionMW::setWorkspace(ws,slicing,false);
  // add other workspaces
  m_offset.resize(m_spectra.size(),0);
  size_t nData = 0;
  for(size_t i = 0; i < m_spectra.size(); ++i)
  {
    mws = m_spectra[i].first;
    size_t n = mws->blocksize();
    m_offset[i] = nData;
    nData += static_cast<int>(n);
  }

  m_data.resize(nData);
  m_weights.resize(nData);

  //... fill in the data and the weights ...

  for(size_t i = 0; i < m_spectra.size(); ++i)
  {
    mws = m_spectra[i].first;
    size_t wi = m_spectra[i].second;
    const Mantid::MantidVec& Y = mws->readY(wi);
    const Mantid::MantidVec& E = mws->readE(wi);
    size_t j0 = m_offset[i];
    for(size_t j = 0; j < Y.size(); ++j)
    {
      m_data[j0 + j] = Y[j];
      double err = E[j];
      m_weights[j0 + j] = err != 0.0 ? 1./err : 1.0;
    }
  }

  //std::cerr << "Workspace:\n";
  //std::for_each(&m_data[0],&m_data[0]+m_dataSize,std::cerr << _1 << '\n');
}

} // namespace API
} // namespace Mantid
