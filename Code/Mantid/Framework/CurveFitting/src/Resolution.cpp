//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Resolution.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/Algorithm.h"
#include <cmath>
#include <fstream>
#include <sstream>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Resolution)

void Resolution::function(double* out, const double* xValues, const int& nData)const
{
  if (nData <= 0) return;

  if (m_xStart >= xValues[nData-1] || m_xEnd <= xValues[0]) return;

  int i = 0;
  while(i < nData - 1 && xValues[i] < m_xStart)
  {
    out[i] = 0;
    i++;
  }
  int j = 0;
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
        out[i] = m_yData[j];
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
      }
      else
      {
        out[i] = 0;
      }
    }
  }
}

/// Returns a list of attribute names
std::vector<std::string> Resolution::getAttributeNames()const
{
  std::vector<std::string> res(1,"FileName");
  return res;
}

/** Set a value to attribute attName
 * @param attName :: The attribute name
 * @param value :: The new value
 */
void Resolution::setAttribute(const std::string& attName,const IFitFunction::Attribute& value)
{
  if (attName == "FileName")
  {
    std::string fileName = value.asUnquotedString();
    FileValidator fval;
    std::string error = fval.isValid(fileName);
    if (error == "")
    {
      m_fileName = fileName;
    }
    else
    {
      //throw std::runtime_error(error);
      return; // allow initialization with invalid attribute (for editing)
    }
    load(m_fileName);
  }
  else
  {
    IFitFunction::setAttribute(attName,value);
  }
}

/**
 * Decide whether to load the file as an ASCII file or as a Nexus file.
 * @param fname :: The file name
 */
void Resolution::load(const std::string& fname)
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

  m_xStart = m_xData.front();
  m_xEnd   = m_xData.back();
}

/**
 * Load input file as an Ascii file.
 * @param fname :: The file name
 */
void Resolution::loadAscii(const std::string& fname)
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
    throw std::runtime_error("Resolution: too few data points");
  }
}

/**
 * Load input file as a Nexus file.
 * @param fname :: The file name
 */
void Resolution::loadNexus(const std::string& fname)
{
  IAlgorithm_sptr loadNxs = Mantid::API::AlgorithmFactory::Instance().create("LoadNexus", -1);
  loadNxs->initialize();
  loadNxs->setChild(true);
  loadNxs->setLogging(false);
  try
  {
    loadNxs->setPropertyValue("Filename", fname);
    loadNxs->setPropertyValue("OutputWorkspace", "_resolution_fit_data_");
    loadNxs->execute();
  }
  catch ( std::runtime_error & )
  {
    throw std::runtime_error("Unable to load Nexus file for resolution function.");
  }
  
  Workspace_sptr ws = loadNxs->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr resData = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
  
  const bool hist = resData->isHistogramData();
  const int nbins = resData->blocksize();

  double first = resData->readX(0)[0];
  double last;
  if ( hist ) { last = resData->readX(0)[nbins]; }
  else { last = resData->readX(0)[nbins-1]; }
  double adjustX = ( first + last ) / 2;


  for ( int i = 0; i < nbins; i++ )
  {
    double x = 0.0;
    m_yData.push_back(resData->readY(0)[i]);
    if ( hist ) x = ( resData->readX(0)[i] + resData->readX(0)[i+1] ) / 2;
    else x = resData->readX(0)[i];
    m_xData.push_back(x-adjustX);
  }
}

} // namespace CurveFitting
} // namespace Mantid
