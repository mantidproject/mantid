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

/// Constructor
TabulatedFunction::TabulatedFunction():
m_xStart(0),m_xEnd(0)
{
  declareParameter("Scaling",1.0,"A scaling factor");
}

/// Evaluate the function for a list of arguments and given scaling factor
void TabulatedFunction::eval(double scaling, double* out, const double* xValues, const size_t nData)const
{
  if (nData == 0 || size() == 0) return;

  if (m_xStart >= xValues[nData-1] || m_xEnd <= xValues[0]) return;

  size_t i = 0;
  while(i < nData - 1 && xValues[i] < m_xStart)
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
void TabulatedFunction::clear()
{
  m_fileName.clear();
  m_wsName.clear();
  m_xData.clear();
  m_yData.clear();
  m_xStart = 0;
  m_xEnd = 0;
}

/// Returns a list of attribute names
std::vector<std::string> TabulatedFunction::getAttributeNames()const
{
  std::vector<std::string> res(2);
  res[0] = "FileName";
  res[1] = "Workspace";
  return res;
}

/** Set a value to attribute attName
 * @param attName :: The attribute name
 * @param value :: The new value
 */
void TabulatedFunction::setAttribute(const std::string& attName,const IFunction::Attribute& value)
{
  std::string aName( attName );
  std::transform(aName.begin(),aName.end(),aName.begin(),toupper);
  if (aName == "FILENAME")
  {
    std::string fileName = value.asUnquotedString();
    FileValidator fval;
    std::string error = fval.isValid(fileName);
    if (error == "")
    {
      m_fileName = fileName;
      m_wsName.clear();
    }
    else
    {
      return; // allow initialization with invalid attribute (for editing)
    }
    load(m_fileName);
  }
  else if (aName == "WORKSPACE") 
  {
    loadWorkspace( value.asString() );
  }
  else
  {
    IFunction::setAttribute(attName,value);
  }
}

/// Return a value of attribute attName
IFunction::Attribute TabulatedFunction::getAttribute(const std::string& attName)const
{
  std::string aName( attName );
  std::transform(aName.begin(),aName.end(),aName.begin(),toupper);
  if ( aName == "FILENAME" )
  {
    return Attribute( m_fileName, true );
  }
  else if ( aName == "WORKSPACE" )
  {
    return Attribute( m_wsName );
  }
  return IFunction::getAttribute( attName );
}

/**
 * Check if attribute attName exists
 * @param attName :: A name to check.
 */
bool TabulatedFunction::hasAttribute(const std::string& attName)const
{
  std::string aName( attName );
  std::transform(aName.begin(),aName.end(),aName.begin(),toupper);
  return aName == "FILENAME" || aName == "WORKSPACE";
}

/**
 * Decide whether to load the file as an ASCII file or as a Nexus file.
 * @param fname :: The file name
 */
void TabulatedFunction::load(const std::string& fname)
{
  m_xData.clear();
  m_yData.clear();

  size_t format = fname.find(".nxs");
  if ( format != std::string::npos )
  {
    loadNexus(fname);
  }
  else
  {
    loadAscii(fname);
  }

}

/**
 * Load input file as an Ascii file.
 * @param fname :: The file name
 */
void TabulatedFunction::loadAscii(const std::string& fname)
{
  std::ifstream fil(fname.c_str());
  std::string str;

  while(getline(fil,str))
  {
    str += ' ';
    std::istringstream istr(str);
    double x,y;
    istr >> x >> y;
    if (!istr.good())
    {
      break;
    }
    m_xData.push_back(x);
    m_yData.push_back(y);
  }

  if (m_xData.size() < 2)
  {
    m_xData.clear();
    m_yData.clear();
    throw std::runtime_error("TabulatedFunction: too few data points");
  }
  m_xStart = m_xData.front();
  m_xEnd   = m_xData.back();
}

/**
 * Load input file as a Nexus file.
 * @param fname :: The file name
 */
void TabulatedFunction::loadNexus(const std::string& fname)
{
  IAlgorithm_sptr loadNxs = Mantid::API::AlgorithmFactory::Instance().create("LoadNexus", -1);
  loadNxs->initialize();
  loadNxs->setChild(true);
  loadNxs->setLogging(false);
  try
  {
    loadNxs->setPropertyValue("Filename", fname);
    loadNxs->setPropertyValue("OutputWorkspace", "_TabulatedFunction_fit_data_");
    loadNxs->execute();
  }
  catch ( std::runtime_error & )
  {
    throw std::runtime_error("Unable to load Nexus file for TabulatedFunction function.");
  }
  
  Workspace_sptr ws = loadNxs->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr resData = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
  loadWorkspace( resData );
  
}

/**
 * Load the points from a MatrixWorkspace
 * @param wsName :: The workspace to load from
 */
void TabulatedFunction::loadWorkspace(const std::string& wsName)
{
  try
  {
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>( wsName );
    loadWorkspace( ws );
    m_wsName = wsName;
    m_fileName.clear();
  }
  catch( ... )
  {
    g_log.error() << "Workspace " << wsName << " not found \n";
    clear();
  }
}

/**
 * Load the points from a MatrixWorkspace
 * @param ws :: The workspace to load from
 */
void TabulatedFunction::loadWorkspace(boost::shared_ptr<MatrixWorkspace> ws)
{
  const bool hist = ws->isHistogramData();
  const size_t nbins = ws->blocksize();

  double last;
  if ( hist ) { last = ws->readX(0)[nbins]; }
  else { last = ws->readX(0)[nbins-1]; }

  for ( size_t i = 0; i < nbins; i++ )
  {
    double x = 0.0;
    m_yData.push_back(ws->readY(0)[i]);
    if ( hist ) x = ( ws->readX(0)[i] + ws->readX(0)[i+1] ) / 2;
    else x = ws->readX(0)[i];
    m_xData.push_back( x );
  }
  m_xStart = m_xData.front();
  m_xEnd   = m_xData.back();
}


} // namespace CurveFitting
} // namespace Mantid
