#ifndef MANTID_API_MODERATORMODEL_H_
#define MANTID_API_MODERATORMODEL_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/ClassMacros.h"

#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif

#include <string>

namespace Mantid
{
  namespace API
  {
    /**
     *
     * Defines a base class for a moderator model
     */
    class MANTID_API_DLL ModeratorModel
    {
    public:
      /// Default constructor required by the factory
      ModeratorModel();
      /// Allow inheritance
      virtual ~ModeratorModel();
      /// Returns a clone of the current object
      virtual boost::shared_ptr<ModeratorModel> clone() const = 0;

      /// Initialize the object from a string of parameters
      void initialize(const std::string & params);
      /// Custom init function called after parameters have been processed. Default action is to do nothing
      virtual void init() {}

      /// Sets the tilt angle in degrees (converted to radians internally)
      void setTiltAngleInDegrees(const double theta);
      /// Returns the value of the tilt angle in radians
      double getTiltAngleInRadians() const;

      /// Returns the mean time for emission in microseconds
      virtual double emissionTimeMean() const = 0;
      /// Returns the variance of emission time in microseconds
      virtual double emissionTimeVariance() const = 0;
      /// Returns a time, in seconds, sampled from the distibution given a flat random number
      virtual double sampleTimeDistribution(const double flatRandomNo) const = 0;

    protected:
      /// Set a named parameter from a string value
      virtual void setParameterValue(const std::string & name, const std::string & value) = 0;

    private:
      /// Moderator tilt angle in radians
      double m_tiltAngle;
    };
  }
}

#endif /* MANTID_API_MODERATORMODEL_H_ */
