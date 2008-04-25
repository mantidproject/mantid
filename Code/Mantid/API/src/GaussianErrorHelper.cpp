#include "MantidAPI/GaussianErrorHelper.h"
#include "MantidKernel/Logger.h"
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
    const void GaussianErrorHelper::plus (const value_type& lhs,const value_type& rhs, value_type& result) const
    { 
      result.Y() = lhs.Y()+rhs.Y();
      result.E() = sqrt((lhs.E()*lhs.E())+(rhs.E()*rhs.E()));
    }

    /**Subtraction
    * @param lhs The lhs value
    * @param rhs The rhs value
    * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
    */
    const void GaussianErrorHelper::minus (const value_type& lhs,const value_type& rhs, value_type& result) const
    {
      result.Y() = lhs.Y()-rhs.Y();
      result.E() = sqrt((lhs.E()*lhs.E())+(rhs.E()*rhs.E()));
    }

    /**Multiplication
    * @param lhs The lhs value
    * @param rhs The rhs value
    * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
    */
    const void GaussianErrorHelper::multiply (const value_type& lhs,const value_type& rhs, value_type& result) const
    {
      result.Y() = lhs.Y()*rhs.Y();
      //  gaussian errors
      // (Sa/a)2 + (Sb/b)2 = (Sc/c)2 
      //  So after taking proportions, squaring, summing, 
      //  and taking the square root, you get a proportional error to the product c.
      //  Multiply that proportional error by c to get the actual standard deviation Sc.  
      result.E() = result.Y()*sqrt(pow((lhs.E()/lhs.Y()),2) + pow((rhs.E()/rhs.Y()),2));   
    } 

    /**Division
    * @param lhs The lhs value
    * @param rhs The rhs value
    * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
    */
    const void GaussianErrorHelper::divide (const value_type& lhs,const value_type& rhs, value_type& result) const
    {
      result.Y() = lhs.Y()/rhs.Y();
      //  gaussian errors
      // (Sa/a)2 + (Sb/b)2 = (Sc/c)2 
      //  So after taking proportions, squaring, summing, 
      //  and taking the square root, you get a proportional error to the product c.
      //  Multiply that proportional error by c to get the actual standard deviation Sc.  
      result.E() = result.Y()*sqrt(pow((lhs.E()/lhs.Y()),2) + pow((rhs.E()/rhs.Y()),2));   
    } 

  } // namespace API
} // namespace Mantid
