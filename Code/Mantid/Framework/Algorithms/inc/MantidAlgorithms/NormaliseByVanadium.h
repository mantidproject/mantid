#ifndef MANTID_ALGORITHMS_NORMALISEBYVANADIUM_H_
#define MANTID_ALGORITHMS_NORMALISEBYVANADIUM_H_
/*WIKI*
Algorithm is used to normalise a workspace against a vanadium run. Averaging of pixels on the vanadium sample is performed using a nearest neighbours search. 
This is suitable for runs in which low statistics have been gathered from the vanadium sample.
*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"


namespace Mantid
{
namespace Algorithms
{

  /** NormaliseByVanadium : Normalise a sample workspace by a vanadium workspace. Use nearest neighbours for averaging vanadium pixels.
    
    @author Owen Arnold
    @date 2011-10-26

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport NormaliseByVanadium : public Mantid::API::Algorithm
  {
  public:

  /// Default constructor
  NormaliseByVanadium();
  /// Destructor
  virtual ~NormaliseByVanadium();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "NormaliseByVanadium";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_NORMALISEBYVANADIUM_H_ */
