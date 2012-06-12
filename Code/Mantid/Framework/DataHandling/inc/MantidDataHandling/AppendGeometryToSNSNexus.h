#ifndef MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_
#define MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{

  /** AppendGeometryToSNSNexus : Appends geometry information to a NeXus file.
    
    @date 2012-06-01

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport AppendGeometryToSNSNexus  : public API::Algorithm
  {
  public:
    AppendGeometryToSNSNexus();
    virtual ~AppendGeometryToSNSNexus();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();

    /// The filename of the NeXus file to append geometry info to
    std::string m_filename;

    /// Instrument name
    std::string m_instrument;

    /// IDF filename
    std::string m_idf_filename;

    /// Get the instrument name from the NeXus file
    std::string getInstrumentName(const std::string & nxfilename);

  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUS_H_ */
