//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"

#include "MantidGeometry/muParser_Silent.h"
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>

#include <sstream>
#include <iostream> 
#include <algorithm>
#include <functional>
#include <iterator>
#include <float.h>

namespace Mantid
{
namespace API
{
  using namespace Geometry;
  
  Kernel::Logger& IFunctionMD::g_log = Kernel::Logger::get("IFunctionMD");

  /** Set the workspace
   * @param ws :: A shared pointer to a workspace.
   */
  void IFunctionMD::setWorkspace(boost::shared_ptr<const Workspace> ws)
  {
    try
    {
      IMDWorkspace_const_sptr workspace = boost::dynamic_pointer_cast<const IMDWorkspace>(ws);
      if (!workspace)
      {
        throw std::invalid_argument("Workspace has a wrong type (not a IMDWorkspace)");
      }

      if (m_dimensionIndexMap.empty())
      {
        useAllDimensions(workspace);
      }

      m_dimensions.resize(m_dimensionIndexMap.size());
      std::map<std::string,size_t>::const_iterator it = m_dimensionIndexMap.begin();
      std::map<std::string,size_t>::const_iterator end = m_dimensionIndexMap.end();
      for(; it != end; ++it)
      {
        boost::shared_ptr<const Mantid::Geometry::IMDDimension> dim = workspace->getDimensionWithId(it->first);
        if (!dim)
        {
          throw std::invalid_argument("Dimension "+it->first+" dos not exist in workspace "+ws->getName());
        }
        m_dimensions[it->second] = dim;
      }

      //if (copyData)
      //{
      //  IMDIterator* r = workspace->createIterator();
      //  m_dataSize = r->getDataSize();

      //  // fill in m_data and m_weights
      //  m_data.reset(new double[m_dataSize]);
      //  m_weights.reset(new double[m_dataSize]);

      //  size_t i = 0;
      //  do
      //  {
      //    double signal = r->getNormalizedSignal(); //point.getSignal();
      //    double error  = r->getNormalizedError();  //point.getError();
      //    if (error == 0) error = 1.;
      //    m_data[i] = signal;
      //    m_weights[i] = 1./error;
      //    i++;
      //  } while(r->next());
      //  delete r;

      //  if (m_dataSize == 0)
      //  {
      //    throw std::runtime_error("Fitting data is empty");
      //  }
      //}

    }
    catch(std::exception& e)
    {
      g_log.error() << "IFunctionMD::setWorkspace failed with error: " << e.what() << '\n';
      throw;
    }

  }

  /// Function you want to fit to. 
  /// @param out :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  void IFunctionMD::function(const FunctionDomain& domain,FunctionValues& values)const
  {
    const FunctionDomainMD* dmd = dynamic_cast<const FunctionDomainMD*>(&domain);
    if (!dmd)
    {
      throw std::invalid_argument("Unexpected domain in IFunctionMD");
    }
    size_t i=0;
    for(const IMDIterator* r = dmd->getNextIterator(); r != NULL;)
    {
      values.setCalculated(i,functionMD(*r));
      i++;
    };
  }


  /** User functions call this method in their constructors to set up the order of the dimensions.
    * The dimensions will be sorted in the order of calls to useDimension. Ordering is needed to
    * access dimensions by ineteger index rather than by name (string)
    * @param id :: The id of a dimension in the workspace
    */
  void IFunctionMD::useDimension(const std::string& id)
  {
    size_t n = m_dimensionIndexMap.size();
    if (m_dimensionIndexMap.find(id) != m_dimensionIndexMap.end())
    {
      throw std::invalid_argument("Dimension "+id+" has already been used.");
    }
    m_dimensionIndexMap[id] = n;
  }

  /**
    * This method is called if a function does not call to useDimension at all. 
    * It adds all the dimensions in the workspace in the order they are in in that workspace
    * then calls init(). 
    */
  void IFunctionMD::useAllDimensions(IMDWorkspace_const_sptr workspace)
  {
    if (!workspace)
    {
      throw std::runtime_error("Method IFunctionMD::useAllDimensions() can only be called after setting the workspace");
    }
    for(size_t i = 0; i < workspace->getNumDims(); ++ i)
    {
      useDimension(workspace->getDimension(i)->getDimensionId());
    }
    this->initDimensions();
  }


} // namespace API
} // namespace Mantid


//#include "MantidAPI/ParamFunction.h"
//#include "MantidKernel/VMD.h"
//
//using Mantid::Kernel::VMD;

//namespace Mantid
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
//        * Defining gaussian's parameters here, ie after the workspace is set and 
//        * the dimensions are known.
//        */
//      void initDimensions()
//      {
//        if (!getWorkspace()) return;
//        std::map<std::string,size_t>::const_iterator it = m_dimensionIndexMap.begin();
//        std::map<std::string,size_t>::const_iterator end = m_dimensionIndexMap.end();
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
//        * @param r :: MD workspace iterator with a reference to the current point
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
//
//    /**
//      * Another example MD function. A function defined as a muParser string.
//      */
//    class UserFunctionMD: public IFunctionMD, public ParamFunction
//    {
//    private:
//      mu::Parser m_parser;
//      mutable std::vector<double> m_vars;
//      std::vector<std::string> m_varNames;
//      std::string m_formula;
//    public:
//      UserFunctionMD()
//      {
//        m_vars.resize(4);
//        std::string varNames[] = {"x","y","z","t"};
//        m_varNames.assign(varNames,varNames+m_vars.size());
//        for(size_t i = 0; i < m_vars.size(); ++i)
//        {
//          m_parser.DefineVar(m_varNames[i],&m_vars[i]);
//        }
//      }
//      bool hasAttribute(const std::string& attName)const 
//      { 
//        UNUSED_ARG(attName);
//        return attName == "Formula";
//      }
//      Attribute getAttribute(const std::string& attName)const
//      {
//        UNUSED_ARG(attName);
//        return Attribute(m_formula);
//      }
//      
//      void setAttribute(const std::string& attName,const Attribute& attr)
//      {
//        UNUSED_ARG(attName);
//        m_formula = attr.asString();
//        if (!m_vars.empty())
//        {
//          setFormula();
//        }
//      }
//      /**
//        * Defining function's parameters here, ie after the workspace is set and 
//        * the dimensions are known.
//        */
//      void initDimensions()
//      {
//        if (!getWorkspace()) return;
//        if (m_vars.size() > 4)
//        {
//          m_vars.resize(m_dimensionIndexMap.size());
//          m_varNames.resize(m_dimensionIndexMap.size());
//          for(size_t i = 0; i < m_vars.size(); ++i)
//          {
//            m_varNames[i] = "x" + boost::lexical_cast<std::string>(i);
//            m_parser.DefineVar(m_varNames[i],&m_vars[i]);
//          }
//        }
//        setFormula();
//      }
//
//      std::string name() const {return "UserFunctionMD";}
//    protected:
//
//      /** 
//        * Calculate the function value at a point r in the MD workspace
//        * @param r :: MD workspace iterator with a reference to the current point
//        */
//      double functionMD(IMDIterator& r) const
//      {
//        size_t n = m_dimensions.size();
//        VMD center = r.getCenter();
//        for(size_t i = 0; i < n; ++i)
//        {
//          m_vars[i] = center[i];
//        }
//        return m_parser.Eval();
//      }
//      /** Static callback function used by MuParser to initialize variables implicitly
//      @param varName :: The name of a new variable
//      @param pufun :: Pointer to the function
//      */
//      static double* AddVariable(const char *varName, void *pufun)
//      {
//        UserFunctionMD& fun = *reinterpret_cast<UserFunctionMD*>(pufun);
//
//        std::vector<std::string>::iterator x = std::find(fun.m_varNames.begin(),fun.m_varNames.end(),varName);
//        if (x != fun.m_varNames.end())
//        {
//          //std::vector<std::string>::difference_type i = std::distance(fun.m_varNames.begin(),x);
//          throw std::runtime_error("UserFunctionMD variables are not defined");
//        }
//        else
//        {
//          try
//          {
//            fun.declareParameter(varName,0.0);
//          }
//          catch(...)
//          {}
//        }
//
//        // The returned pointer will never be used. Just returning a valid double pointer
//        return &fun.m_vars[0];
//      }
//
//      /**
//        * Initializes the mu::Parser.
//        */
//      void setFormula()
//      {
//        // variables must be already defined
//        if (m_vars.empty()) return;
//        if (m_formula.empty())
//        {
//          m_formula = "0";
//        }
//        m_parser.SetVarFactory(AddVariable,this);
//        m_parser.SetExpr(m_formula);
//        // declare function parameters using mu::Parser's implicit variable setting
//        m_parser.Eval();
//        m_parser.ClearVar();
//        // set muParser variables
//        for(size_t i = 0; i < m_vars.size(); ++i)
//        {
//          m_parser.DefineVar(m_varNames[i],&m_vars[i]);
//        }
//        for(size_t i=0;i<nParams();i++)
//        {
//          m_parser.DefineVar(parameterName(i),getParameterAddress(i));
//        }
//
//        m_parser.SetExpr(m_formula);
//      }
//
//    };

    // Subscribe the function into the factory.
    //DECLARE_FUNCTION(UserFunctionMD);

//  } // API
//}   // Mantid

