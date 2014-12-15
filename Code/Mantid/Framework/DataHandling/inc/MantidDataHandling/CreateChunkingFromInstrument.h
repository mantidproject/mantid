#ifndef MANTID_DATAHANDLING_CREATECHUNKINGFROMINSTRUMENT_H_
#define MANTID_DATAHANDLING_CREATECHUNKINGFROMINSTRUMENT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace DataHandling
{

  /** CreateChunkingFromInstrument : TODO: DESCRIPTION
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport CreateChunkingFromInstrument  : public API::Algorithm
  {
  public:
    CreateChunkingFromInstrument();
    virtual ~CreateChunkingFromInstrument();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;
    virtual std::map<std::string, std::string> validateInputs();

  private:
    void init();
    void exec();
    Geometry::Instrument_const_sptr getInstrument();
  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_CREATECHUNKINGFROMINSTRUMENT_H_ */
