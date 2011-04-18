#ifndef MANTID_PYTHONAPI_BOOSTPYTHON_SILENT_H_
#define MANTID_PYTHONAPI_BOOSTPYTHON_SILENT_H_

/** 
    This file is a wrapper for the boost python header as it 
    produces warnings that are beyond our control.

    Please include this file rather than boost/python.hpp

    @author Martyn Gigg Tessella plc

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

*/

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4244)
  #pragma warning(disable:4267)
  #pragma warning(disable:4250)
  #include <boost/python.hpp>
  #include <boost/python/suite/indexing/vector_indexing_suite.hpp>
  #pragma warning(pop)
#elif defined __GNUC__
  #pragma GCC system_header
#endif


#endif // MANTID_PYTHONAPI_BOOSTPYTHON_SILENT_H_