#include "MantidAPI/GaussianErrorHelper.h"
#include <cmath>

using namespace Mantid::Kernel;

namespace Mantid
{
  namespace API
  {
    Kernel::Logger& GaussianErrorHelper::g_log = Kernel::Logger::get("GaussianErrorHelper");
    GaussianErrorHelper* GaussianErrorHelper::m_instance = 0;

    
    /** A static method which retrieves the single instance of the Algorithm Manager
    * 
    *  @returns A pointer to the Algorithm Manager instance
    */
    GaussianErrorHelper* GaussianErrorHelper::Instance()
    {
      if (!m_instance) m_instance = new GaussianErrorHelper;	 		
      return m_instance;
    }

    /// Private Constructor for singleton class
    GaussianErrorHelper::GaussianErrorHelper()
    {
    }

    /**Addition
    * @param lhs The lhs value
    * @param rhs The rhs value
    * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
    */
    const GaussianErrorHelper::value_type GaussianErrorHelper::plus (const value_type& lhs,const value_type& rhs) const
    { 
      xVal = lhs[0];
      yVal = lhs[1]+rhs[1];
      err = sqrt((lhs[2]*lhs[2])+(rhs[2]*rhs[2]));
      return value_type(xVal,yVal,err);   
    }

    /**Subtraction
    * @param lhs The lhs value
    * @param rhs The rhs value
    * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
    */
    const GaussianErrorHelper::value_type GaussianErrorHelper::minus (const value_type& lhs,const value_type& rhs) const
    {
      xVal = lhs[0];
      yVal = lhs[1]-rhs[1];
      err = sqrt((lhs[2]*lhs[2])+(rhs[2]*rhs[2]));
      return value_type(xVal,yVal,err);   
    }

    /**Multiplication
    * @param lhs The lhs value
    * @param rhs The rhs value
    * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
    */
    const GaussianErrorHelper::value_type GaussianErrorHelper::multiply (const value_type& lhs,const value_type& rhs) const
    {
      xVal = lhs[0];
      yVal = lhs[1]*rhs[1];
      //  gaussian errors
      // (Sa/a)2 + (Sb/b)2 = (Sc/c)2 
      //  So after taking proportions, squaring, summing, 
      //  and taking the square root, you get a proportional error to the product c.
      //  Multiply that proportional error by c to get the actual standard deviation Sc.  
      err = yVal*sqrt(pow((lhs[2]/lhs[1]),2) + pow((rhs[2]/rhs[1]),2));   
      return value_type(xVal,yVal,err);    
    } 

    /**Division
    * @param lhs The lhs value
    * @param rhs The rhs value
    * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
    */
    const GaussianErrorHelper::value_type GaussianErrorHelper::divide (const value_type& lhs,const value_type& rhs) const
    {
      xVal = lhs[0];
      yVal = lhs[1]/rhs[1];
      //  gaussian errors
      // (Sa/a)2 + (Sb/b)2 = (Sc/c)2 
      //  So after taking proportions, squaring, summing, 
      //  and taking the square root, you get a proportional error to the product c.
      //  Multiply that proportional error by c to get the actual standard deviation Sc.  
      err = yVal*sqrt(pow((lhs[2]/lhs[1]),2) + pow((rhs[2]/rhs[1]),2)); 
      return value_type(xVal,yVal,err);    
    } 

  } // namespace API
} // namespace Mantid
