//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/CompositeFunctionMD.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lambda/lambda.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <float.h>

using namespace boost::lambda;

namespace Mantid
{
namespace API
{

DECLARE_FUNCTION(CompositeFunctionMD)

/// Copy contructor
//CompositeFunctionMD::CompositeFunctionMD(const CompositeFunctionMD& f)
//:CompositeFunction(f),IFunctionMD(f)
//{
//}

///Destructor
CompositeFunctionMD::~CompositeFunctionMD()
{
}

/// Function you want to fit to.
void CompositeFunctionMD::function(double* out)const
{
  boost::scoped_array<double> tmpOut(new double[dataSize()]);
  std::fill_n(out,m_dataSize,0);
  for(size_t i = 0; i < nFunctions(); i++)
  {
    //IFunctionMD* fun = dynamic_cast<IFunctionMD*>(getFunction(i));
    IFitFunction* fun = dynamic_cast<IFitFunction*>(getFunction(i));
    if (!fun)
    {
      throw std::runtime_error("IFitFunction expected but found other type found");
    }
    size_t nWS =  m_wsIndex[i].size();
    for(size_t k = 0;k < nWS; ++k)
    {
      fun->setWorkspace(m_workspaces[k],"",false);
      size_t j = m_wsIndex[i][k];
      double *out1 = out + m_offset[j];
      double *tmp1 = tmpOut.get() + m_offset[j];
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
void CompositeFunctionMD::functionDeriv(Jacobian* out)
{
    // it is possible that out is NULL
    if (!out) return;
    // claculate numerically
    double stepPercentage = DBL_EPSILON*1000; // step percentage
    double step; // real step
    const size_t nParam = nParams();
    const size_t nData  = dataSize();

    // allocate memory if not already done
    if (!m_tmpFunctionOutputMinusStep && nData>0)
    {
      m_tmpFunctionOutputMinusStep.reset(new double[nData]);
      m_tmpFunctionOutputPlusStep.reset(new double[nData]);
    }

    function(m_tmpFunctionOutputMinusStep.get());

    for (size_t iP = 0; iP < nParam; iP++)
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
        function(m_tmpFunctionOutputPlusStep.get());

        step = paramPstep - val;
        setParameter(iP, val);

        for (size_t i = 0; i < nData; i++) 
	{
          double value = (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/step;
          out->set(i,iP,value);
        }
      }
    }
}

void CompositeFunctionMD::setWorkspace(boost::shared_ptr<const Workspace> ws,const std::string& slicing, bool copyData)
{
  UNUSED_ARG(copyData) // Fix copyData = TRUE, effectively
  boost::shared_ptr<const IMDWorkspace> mws = boost::dynamic_pointer_cast<const IMDWorkspace>(ws);
  if (ws && !mws)
  {
    throw std::invalid_argument("Workspace has a wrong type (not a IMDWorkspace)");
  }

  if (mws)
  {
    m_workspaces.resize(1,mws);
  }
  m_wsIndex.resize(nFunctions());

  // Go through every function in this composite
  for(size_t iFun=0;iFun<nFunctions();iFun++)
  {
    IFitFunction* fun = dynamic_cast<IFitFunction*>(getFunction(iFun));
    if (!fun)
    {
      throw std::runtime_error("IFitFunction expected but found other type found");
    }
    if (fun->getWorkspace())
    {
      // Function already had a workspace associated to it
      boost::shared_ptr<const IMDWorkspace> iws =  boost::dynamic_pointer_cast<const IMDWorkspace>(fun->getWorkspace());
      std::vector< boost::shared_ptr<const IMDWorkspace> >::iterator it = std::find(m_workspaces.begin(),m_workspaces.end(),iws);
      size_t i;
      if (it == m_workspaces.end())
      {
        // It was not already in the list - add it to our list of workspaces
        i = m_workspaces.size();
        m_workspaces.push_back(iws);
      }
      else
      {
        // Find the index of this workspace in our list
        i = size_t(std::distance(it,m_workspaces.begin()));
      }
      // Associate the workspace #i with the function #iFun
      m_wsIndex[iFun].push_back(i);
      // And set it on the function
      fun->setWorkspace(boost::static_pointer_cast<const Workspace>(iws),slicing,false);
    }
    //else
    //{
    //  fun->setWorkspace(ws,slicing,false);
    //}
  }

  // If the workspace #s have not been set for each function, set them
  for(size_t iFun=0;iFun<nFunctions();iFun++)
  {
    std::vector<size_t>& index = m_wsIndex[iFun];
    if (index.empty())
    {
      index.resize(m_workspaces.size());
      size_t i = 0;
      std::for_each(index.begin(),index.end(),_1 = var(i)++);
    }
  }

  // Set dimensions. This clears m_dataSize to 0.
  IFitFunction::setWorkspace(ws,slicing,false);

  // Cache the iterators
  std::vector<IMDIterator*> iterators;
  for(size_t i = 0; i < m_workspaces.size(); ++i)
    iterators.push_back(m_workspaces[i]->createIterator());

  // Add ALL the workspaces
  m_dataSize = 0;
  m_offset.resize(m_workspaces.size(),0);
  for(size_t i = 0; i < m_workspaces.size(); ++i)
  {
    mws = m_workspaces[i];
    IMDIterator* r = iterators[i];
    size_t n = r->getDataSize();
    m_offset[i] = m_dataSize;
    m_dataSize += static_cast<int>(n); // increase the size of it
  }

  m_data.reset(new double[m_dataSize]);
  m_weights.reset(new double[m_dataSize]);

  //... fill in the data and the weights ...

  for(size_t i = 0; i < m_workspaces.size(); ++i)
  {
    mws = m_workspaces[i];
    IMDIterator* it = iterators[i];
    size_t j0 = m_offset[i];
    do
    {
      m_data[j0] = it->getNormalizedSignal();
      double err = it->getNormalizedError();
      m_weights[j0] = err != 0.0 ? 1./err : 1.0;
      j0++;
    }while(it->next());
    delete it;
  }

  //std::cerr << "Workspace:\n";
  //std::for_each(&m_data[0],&m_data[0]+m_dataSize,std::cerr << _1 << '\n');
}

double CompositeFunctionMD::functionMD(IMDIterator& r) const
{
  UNUSED_ARG(r)
  return 0;
}

} // namespace API
} // namespace Mantid
