//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IConstraint.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"

#include "boost/lexical_cast.hpp"

#include <sstream>
#include <iostream> 
#include <algorithm>
#include <functional>
#include <float.h>

namespace Mantid
{
namespace API
{
  using namespace Geometry;
  
  Kernel::Logger& IFunctionMD::g_log = Kernel::Logger::get("IFunctionMD");

  /** Set the workspace
    * @param ws A shared pointer to a workspace. Must be a MatrixWorkspace.
    * @param slicing A string identifying the data to be fitted. 
  */
  void IFunctionMD::setWorkspace(boost::shared_ptr<Workspace> ws,const std::string& slicing)
  {
    try
    {
      m_workspace = boost::dynamic_pointer_cast<IMDWorkspace>(ws);
      if (!m_workspace)
      {
        throw std::invalid_argument("Workspace has a wrong type (not a IMDWorkspace)");
      }

      if (m_dimensionIndexMap.empty())
      {
        useAllDimensions();
      }

      m_dataSize = 1;
      m_dimensions.resize(m_dimensionIndexMap.size());
      std::map<std::string,int>::const_iterator it = m_dimensionIndexMap.begin();
      std::map<std::string,int>::const_iterator end = m_dimensionIndexMap.end();
      for(; it != end; ++it)
      {
        boost::shared_ptr<const Mantid::Geometry::IMDDimension> dim = m_workspace->getDimension(it->first);
        if (!dim)
        {
          throw std::invalid_argument("Dimension "+it->first+" dos not exist in workspace "+ws->getName());
        }
        m_dimensions[it->second] = dim;
        m_dataSize *= dim->getNBins();
      }

      if (m_dataSize == 0)
      {
        throw std::runtime_error("Fitting data is empty");
      }

      // fill in m_data and m_weights
      m_data.reset(new double[m_dataSize]);
      m_weights.reset(new double[m_dataSize]);

      MDIterator from(m_workspace);
      MDIterator to(this);
      if (from.getDataSize() != to.getDataSize())
      {
        throw std::runtime_error("Function must be defined on all workspace");
      }

      do
      {
        const Mantid::Geometry::SignalAggregate& point = m_workspace->getPoint(from.getPointer());
        double signal = point.getSignal();
        double error  = point.getError();
        if (error == 0) error = 1.;
        to.setData(&m_data[0],signal);
        to.setData(&m_weights[0],1./error);
      }while(from.next() && to.next());
    }
    catch(std::exception& e)
    {
      g_log.error() << "IFunctionMD::setWorkspace failed with error: " << e.what() << '\n';
      throw;
    }

  }

  /// Get the workspace
  boost::shared_ptr<const Workspace> IFunctionMD::getWorkspace()const
  {
    return m_workspace;
  }

  /// Returns the size of the fitted data (number of double values returned by the function)
  int IFunctionMD::dataSize()const
  {
    return m_dataSize;
  }

  /// Returns a pointer to the fitted data. These data are taken from the workspace set by setWorkspace() method.
  const double* IFunctionMD::getData()const
  {
    return &m_data[0];
  }

  const double* IFunctionMD::getWeights()const
  {
    return &m_weights[0];
  }

  /// Function you want to fit to. 
  /// @param out The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  void IFunctionMD::function(double* out)const
  {
    if (m_dataSize == 0) return;
    MDIterator r(this);
    do
    {
      r.setData(out,function(r));
    }while(r.next());
  }

  /// Derivatives of function with respect to active parameters
  void IFunctionMD::functionDeriv(Jacobian* out)
  {
    // it is possible that out is NULL
    if (!out) return;
    // claculate numerically
    double stepPercentage = DBL_EPSILON*1000; // step percentage
    double step; // real step
    double minDouble = std::numeric_limits<double>::min();
    double cutoff = 100.0*minDouble/stepPercentage;
    const int nParam = nParams();
    const int nData  = dataSize();

    // allocate memory if not already done
    if (!m_tmpFunctionOutputMinusStep && nData>0)
    {
      m_tmpFunctionOutputMinusStep.reset(new double[nData]);
      m_tmpFunctionOutputPlusStep.reset(new double[nData]);
    }

    function(m_tmpFunctionOutputMinusStep.get());

    for (int iP = 0; iP < nParam; iP++)
    {
      if ( isActive(iP) )
      {
        const double& val = getParameter(iP);
        if (val == 0.0)
        {
          step = stepPercentage;
        }
        else if (val < cutoff)
        {
          step = cutoff;
        }
        else
        {
          step = val*stepPercentage;
        }

        //double paramMstep = val - step;
        //setParameter(iP, paramMstep);
        //function(m_tmpFunctionOutputMinusStep.get());

        double paramPstep = val + step;
        setParameter(iP, paramPstep);
        function(m_tmpFunctionOutputPlusStep.get());

        step = paramPstep - val;
        setParameter(iP, val);

        for (int i = 0; i < nData; i++) {
         // out->set(i,iP, 
         //   (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/(2.0*step));
          out->set(i,iP, 
            (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/step);
        }
      }
    }
  }

  /**
    * Constructor.
    * @param ws A pointer to the iterated workspace.
    */
  IFunctionMD::MDIterator::MDIterator(boost::shared_ptr<const IMDWorkspace> ws):
  m_workspace(ws),
  m_data_pointer(0),
  m_dataSize(0)
  {
    std::vector<std::string> ids = ws->getDimensionIDs();
    for(int i = 0; i < ids.size(); ++i)
    {
      m_dimensions.push_back(ws->getDimension(ids[i]));
    }
    m_index.resize(m_dimensions.size());
    std::fill(m_index.begin(),m_index.end(),0);
  }

  /**
    * Constructor.
    * @param ws A pointer to a MD funtion.
    */
  IFunctionMD::MDIterator::MDIterator(const IFunctionMD* fun):
  m_workspace(fun->m_workspace),
  m_dimensions(fun->m_dimensions),
  m_index(fun->m_dimensions.size()),
  m_data_pointer(0),
  m_dataSize(0)
  {
  }

  /**
    * Returns the total size of the data in the workspace
    */
  int IFunctionMD::MDIterator::getDataSize()const
  {
    if (m_dataSize == 0)
    {
      m_dataSize = 1;
      for(int i = 0; i< m_dimensions.size(); ++i)
      {
        m_dataSize *= m_dimensions[i]->getNBins();
      }
    }
    return m_dataSize;
  }

  /**
    * Get the value of the i-th coordinate at the current point.
    * @param i Index of the dimension.
    */
  double IFunctionMD::MDIterator::getAxisValue(int i)const
  {
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dim = m_dimensions[i];
    return dim->getX(m_index[i]);
  }

  /**
    * Return the current value in a data array
    * @param data A pointer to the first element of a data array. 
    */
  double IFunctionMD::MDIterator::getData(const double* data)const
  {
    return data[m_data_pointer];
  }

  /**
    * Set a value to an element of data pointed to by this iterator
    * @param data A pointer to the first element of a data array. 
    * @param value A value to set.
    */
  void IFunctionMD::MDIterator::setData(double* data,const double& value)const
  {
    data[m_data_pointer] = value;
  }

  /**
    * Increments the iterator to the next MD point. Stops when the last dimension reaches its last value.
    */
  bool IFunctionMD::MDIterator::next()
  {
    bool overflow = false;
    for(int i=0;i<m_index.size();++i)
    {
      int j = m_index[i] + 1;
      overflow = (j == m_dimensions[i]->getNBins());
      if (overflow)
      {
        m_index[i] = 0;
      }
      else
      {
        m_index[i] = j;
        break;
      }
    }
    if (overflow)
    {
      m_index.back() = m_dimensions.back()->getNBins() - 1; // to be safe
      m_data_pointer = m_dataSize - 1;
      return false;
    }
    ++m_data_pointer;
    return true;
  }

  /** User functions call this method in their constructors to set up the order of the dimensions.
    * @param id The id of a dimension in the workspace
    */
  void IFunctionMD::useDimension(const std::string& id)
  {
    int n = m_dimensionIndexMap.size();
    if (m_dimensionIndexMap.find(id) != m_dimensionIndexMap.end())
    {
      throw std::invalid_argument("Dimension "+id+" has already been used.");
    }
    m_dimensionIndexMap[id] = n;
  }

  void IFunctionMD::useAllDimensions()
  {
    if (!m_workspace)
    {
      throw std::runtime_error("Method IFunctionMD::useAllDimensions() can only be called after setting the workspace");
    }
    std::vector<std::string> ids = m_workspace->getDimensionIDs();
    for(int i = 0; i < ids.size(); ++ i)
    {
      useDimension(ids[i]);
    }
    this->init();
  }


} // namespace API
} // namespace Mantid


#include "MantidAPI/ParamFunction.h"

namespace Mantid
{
  namespace API
  {

    class GaussianMD: public IFunctionMD, public ParamFunction
    {
    public:
      void init()
      {
        if (!getWorkspace()) return;
        std::map<std::string,int>::const_iterator it = m_dimensionIndexMap.begin();
        std::map<std::string,int>::const_iterator end = m_dimensionIndexMap.end();
        for(; it != end; ++it)
        {
          declareParameter(it->first+"_centre",0.0);
          declareParameter(it->first+"_alpha",1.0);
        }
        declareParameter("Height",1.0);
      }
      std::string name() const {return "GaussianMD";}
    protected:
      double function(MDIterator& r) const
      {
        double arg = 0.0;
        int n = m_dimensions.size();
        for(int i = 0; i < n; ++i)
        {
          double c = getParameter(2*i);
          double a = getParameter(2*i + 1);
          double t = r.getAxisValue(i) - c;
          arg += a*t*t;
        }
        return getParameter("Height") * exp(-arg);
      }
    };

    DECLARE_FUNCTION(GaussianMD);

  }
}

