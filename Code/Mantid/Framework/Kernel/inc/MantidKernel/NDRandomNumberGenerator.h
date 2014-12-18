#ifndef MANTID_KERNEL_NDRANDOMNUMBERGENERATOR_H_
#define MANTID_KERNEL_NDRANDOMNUMBERGENERATOR_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "DllConfig.h"
#include "ClassMacros.h"
#include <vector>

namespace Mantid
{
  namespace Kernel
  {
    /** 
      This class defines an interface for N dimensional random number generators.
      A call to next produces N points in an ND space

      @author Martyn Gigg, Tessella plc
      @date 19/05/2012

      Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
      
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
      
      File change history is stored at: <https://github.com/mantidproject/mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class MANTID_KERNEL_DLL NDRandomNumberGenerator
    {
    public:
      /// Constructor
      NDRandomNumberGenerator(const unsigned int ndims);
      /// Virtual destructor to ensure that all inheriting classes have one
      virtual ~NDRandomNumberGenerator() {};

      /// Returns the number of dimensions the point will be generated in, i.e. the size
      /// of the vector returned from by nextPoint()
      inline unsigned int numberOfDimensions() const { return m_ndims; }
      /// Generate the next set of values that form a point in ND space
      const std::vector<double> & nextPoint();

      /// Restarts the generator from the beginning of the sequence
      virtual void restart() = 0;
      /// Saves the current state of the generator
      virtual void save() = 0;
      /// Restores the generator to the last saved point, or the beginning if nothing has been saved
      virtual void restore() = 0;

    protected:
      /// Generate the next point. Override this in you concrete implementation
      virtual void generateNextPoint() = 0;

      /// Cache a value for a given dimension index, i.e. 0->ND-1
      void cacheGeneratedValue(const size_t index, const double value);
      /// Cache the while point in one go
      void cacheNextPoint(const std::vector<double> & nextPoint);
      /// Some generators need direct access to the cache
      inline std::vector<double> & getNextPointCache() { return m_nextPoint; }

    private:
      DISABLE_DEFAULT_CONSTRUCT(NDRandomNumberGenerator);
      DISABLE_COPY_AND_ASSIGN(NDRandomNumberGenerator);

      /// The number of dimensions
      const unsigned int m_ndims;
      /// Storage the next point to return
      std::vector<double> m_nextPoint;
    };
  }
}

#endif
