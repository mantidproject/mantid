#ifndef MANTID_KERNEL_MERSENNE_TWISTER_H_
#define MANTID_KERNEL_MERSENNE_TWISTER_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "RandomNumberGenerator.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

namespace Mantid
{
  namespace Kernel
  {
    /** 
      This implements the the Mersenne Twister 19937 pseudo-random number 
      generator algorithm as a specialzation of the RandomNumberGenerator
      interface.

      Further documentation can be found here:
      
      http://www.boost.org/doc/libs/1_42_0/libs/random/random-generators.html

      @author Martyn Gigg, Tessella plc
      @date 19/11/2007

      Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
      This file is part of Mantid.
      
      Mantid is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 3 of the License, or
      (at your option) any later version.
      
      Mantid is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.
      
      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>.
      
      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class EXPORT_OPT_MANTID_KERNEL MersenneTwister : public RandomNumberGenerator
    {
      /// Typedef for a uniform distribution of doubles
      typedef boost::uniform_real<double> uniform_double;
      /// Typedef for a variate generator tieing together the Mersenne Twister 
      /// algorithm with a uniform disribution
      typedef boost::variate_generator<boost::mt19937&, 
	boost::uniform_real<double> > uniform_generator;      
    
    public:
      /// Default constructor
      MersenneTwister();
      /// Set the random number seed
      virtual void setSeed(unsigned int seed);
      /// Sets the range of the subsequent calls to next 
      virtual void setRange(double start, double end);
      /// Generate the next random number in the sequence within the given range, (default=[0.0,1.0]).
      virtual double next();

    private:
      /// The boost Mersenne Twister generator
      boost::mt19937 m_generator;
      /// A boost distribution class to produce uniform numbers between a range
      uniform_double m_uniform_dist;
    };

  }
}


#endif
