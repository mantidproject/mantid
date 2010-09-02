//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Resolution.h"
#include "MantidKernel/FileValidator.h"
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
 * @param attName The attribute name
 * @param value The new value
 */
void Resolution::setAttribute(const std::string& attName,const IFunction::Attribute& value)
{
  if (attName == "FileName")
  {
    std::string fileName = value.asString();
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
    IFunction::setAttribute(attName,value);
  }
}

/**
 * Load the resolution data from an ascii file. The file has three columns with
 * x,y and ignored data.
 * @param fname The file name
 */
void Resolution::load(const std::string& fname)
{
  std::ifstream fil(fname.c_str());
  std::string str;
  m_xData.clear();
  m_yData.clear();
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
    throw std::runtime_error("Resolution: too few data points");
  }

  m_xStart = m_xData.front();
  m_xEnd   = m_xData.back();

}

} // namespace CurveFitting
} // namespace Mantid
