#ifndef MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACE_H_
#define MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include <deque>

namespace Mantid
{
namespace MDEvents
{

  /** ImportMDEventWorkspace : TODO: DESCRIPTION
    
    @date 2012-07-11

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
  class DLLExport ImportMDEventWorkspace  : public API::Algorithm
  {
  public:
    ImportMDEventWorkspace();
    virtual ~ImportMDEventWorkspace();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

    /// Flag used to indicate the dimension block in the file
    static const std::string DimensionBlockFlag();
    /// Flag used to indicate the mdevent block in the file
    static const std::string MDEventBlockFlag();
  
  private:
    /// Typdef for the white-space separated file data type.
    typedef std::deque<std::string> DataCollectionType;
    /// All read-in data.
    DataCollectionType m_file_data;
    /// Iterator for the dimensionality start position.
    DataCollectionType::iterator m_posDimStart;
    /// Iterator for the mdevent data start position.
    DataCollectionType::iterator m_posMDEventStart;
    /// Possible Event Types
    enum MDEventType{Lean, Full, NotSpecified};
    /// Flag indicating whether full md events for lean events will be generated.
    bool m_IsFullMDEvents;

    /// call back to add event data
    template<typename MDE, size_t nd>
    void addEventData(typename MDEventWorkspace<MDE, nd>::sptr ws);
    /// Read the proposed Event Type.
    MDEventType readEventFlag();
    /// Quick check of the structure, so we can abort if passed junk.
    void quickFileCheck();
    ///  Check that the a flag exists in the file.
    bool fileDoesContain(const std::string& flag);
    /// Initialize documentation
    virtual void initDocs();
    void init();
    void exec();
  };



} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACE_H_ */