#ifndef MANTIDQTCUSTOMINTERFACES_LATTICE_VIEW_H
#define MANTIDQTCUSTOMINTERFACES_LATTICE_VIEW_H

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** Abstract lattice view.
  
      @author Owen Arnold, RAL ISIS
      @date 06/Oct/2011

      Copyright &copy; 2010-11 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class LatticeView 
    {
    public:
      virtual double getA1() const = 0;
      virtual double getA2() const = 0;
      virtual double getA3() const = 0;
      virtual double getB1() const = 0;
      virtual double getB2() const = 0;
      virtual double getB3() const = 0;
      virtual void indicateModified() = 0;
      virtual void indicateDefault() = 0;
      virtual void indicateInvalid() = 0;
      virtual void initalize(double a1, double a2, double a3, double b1, double b2, double b3) = 0;
    };
  }
}

#endif