//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/TabulatedFunction.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AnalysisDataService.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(TabulatedFunction)

const int TabulatedFunction::defaultIndexValue = 0;

/// Constructor
TabulatedFunction::TabulatedFunction():
    m_setupFinished(false)
{
  declareParameter("Scaling",1.0,"A scaling factor");
  declareAttribute("FileName", Attribute("", true));
  declareAttribute("Workspace", Attribute(""));
  declareAttribute("WorkspaceIndex", Attribute(defaultIndexValue));
}

/// Evaluate the function for a list of arguments and given scaling factor
void TabulatedFunction::eval(double scaling, double* out, const double* xValues, const size_t nData)const
{
  if (nData == 0) return;

  setupData();

  if (size() == 0) return;

  const double xStart = m_xData.front();
  const double xEnd = m_xData.back();

  if (xStart >= xValues[nData-1] || xEnd <= xValues[0]) return;

  size_t i = 0;
  while(i < nData - 1 && xValues[i] < xStart)
  {
    out[i] = 0;
    i++;
  }
  size_t j = 0;
  for(;i<nData;i++)
  {
    if (j >= size()-1)
    {
      out[i] = 0;
    }
    else
    {
      double xi = xValues[i];
      while(j < size()-1 && xi > m_xData[j]) j++;
      if (xi == m_xData[j])
      {
        out[i] = m_yData[j] * scaling;
      }
      else if (j == size()-1)
      {
        out[i] = 0;
      }
      else if (j > 0)
      {
        double x0 = m_xData[j-1];
        double x1 = m_xData[j];
        double y0 = m_yData[j-1];
        double y1 = m_yData[j];
        out[i] = y0 + (y1 - y0)*(xi - x0)/(x1 - x0);
        out[i] *= scaling;
      }
      else
      {
        out[i] = 0;
      }
    }
  }
}

/**
 * Calculate the function values.
 * @param out :: The output buffer for the calculated values.
 * @param xValues :: The array of x-values.
 * @param nData :: The size of the data.
 */
void TabulatedFunction::function1D(double* out, const double* xValues, const size_t nData)const
{
  const double scaling = getParameter(0);
  eval(scaling, out, xValues, nData);
}

/**
 * function derivatives
 * @param out :: The output Jacobian matrix: function derivatives over its parameters.
 * @param xValues :: The function arguments
 * @param nData :: The size of xValues.
 */
void TabulatedFunction::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
{
  std::vector<double> tmp( nData );
  eval(1.0, tmp.data(), xValues, nData);
  for(size_t i = 0; i < nData; ++i)
  {
    out->set( i, 0, tmp[i] );
  }
}


/// Clear all data
void TabulatedFunction::clear() const
{
  m_xData.clear();
  m_yData.clear();
  m_setupFinished = false;
}

/** Set a value to attribute attName
 * @param attName :: The attribute name
 * @param value :: The new value
 */
void TabulatedFunction::setAttribute(const std::string& attName,const IFunction::Attribute& value)
{
  if (attName == "FileName")
  {
    std::string fileName = value.asUnquotedString();
    FileValidator fval;
    std::string error = fval.isValid(fileName);
    if (error == "")
    {
        storeAttributeValue(attName, Attribute(fileName,true));
        storeAttributeValue("Workspace", Attribute(""));
    }
    else
    {
        // file not found
        throw Kernel::Exception::FileError( error, fileName );
    }
    load(fileName);
  }
  else if (attName == "Workspace")
  {
    storeAttributeValue( attName, value );
    storeAttributeValue( "FileName", Attribute("",true));
    loadWorkspace( value.asString() );
  }
  else
  {
    IFunction::setAttribute(attName,value);
    m_setupFinished = false;
  }
}

/**
 * Load input file as a Nexus file.
 * @param fname :: The file name
 */
void TabulatedFunction::load(const std::string& fname)
{
  IAlgorithm_sptr loadAlg = Mantid::API::AlgorithmFactory::Instance().create("Load", -1);
  loadAlg->initialize();
  loadAlg->setChild(true);
  loadAlg->setLogging(false);
  try
  {
    loadAlg->setPropertyValue("Filename", fname);
    loadAlg->setPropertyValue("OutputWorkspace", "_TabulatedFunction_fit_data_");
    loadAlg->execute();
  }
  catch ( std::runtime_error & )
  {
    throw std::runtime_error("Unable to load Nexus file for TabulatedFunction function.");
  }
  
  Workspace_sptr ws = loadAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr resData = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
  loadWorkspace( resData );
  
}

/**
 * Load the points from a MatrixWorkspace
 * @param wsName :: The workspace to load from
 */
void TabulatedFunction::loadWorkspace(const std::string& wsName) const
{
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>( wsName );
    loadWorkspace( ws );
}

/**
 * Load the points from a MatrixWorkspace
 * @param ws :: The workspace to load from
 */
void TabulatedFunction::loadWorkspace(boost::shared_ptr<API::MatrixWorkspace> ws) const
{
  m_workspace = ws;
  m_setupFinished = false;
}

/**
  * Fill in the x and y value containers (m_xData and m_yData)
  */
void TabulatedFunction::setupData() const
{
    if ( m_setupFinished )
    {
        g_log.debug() << "Re-setting isn't required.";
        return;
    }

    if ( !m_workspace )
    {
        std::string wsName = getAttribute("Workspace").asString();
        if ( wsName.empty() ) throw std::invalid_argument("Data not set for function " + this->name());
        else
            loadWorkspace( wsName );
    }

    size_t index = static_cast<size_t>(getAttribute("WorkspaceIndex").asInt());

    g_log.debug() << "Setting up " << m_workspace->name() << " index " << index << std::endl;

    const bool hist = m_workspace->isHistogramData();
    const size_t nbins = m_workspace->blocksize();
    m_xData.resize(nbins);
    m_yData.resize(nbins);

    for ( size_t i = 0; i < nbins; i++ )
    {
      double x = 0.0;
      m_yData[i] = m_workspace->readY(index)[i];
      auto &xvec = m_workspace->readX(index);
      if ( hist ) x = ( xvec[i] + xvec[i+1] ) / 2;
      else x = xvec[i];
      m_xData[i] = x;
    }

    m_workspace.reset();
    m_setupFinished = true;
}

} // namespace CurveFitting
} // namespace Mantid
