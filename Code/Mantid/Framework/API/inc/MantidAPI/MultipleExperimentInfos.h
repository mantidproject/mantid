#ifndef MANTID_API_MULTIPLEEXPERIMENTINFOS_H_
#define MANTID_API_MULTIPLEEXPERIMENTINFOS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ExperimentInfo.h"


namespace Mantid
{
namespace API
{

  /** Small class that allows a MDEventWorkspace or a MDHistoWorkspace
    to hold several ExperimentInfo classes.
    
    @date 2011-11-28

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport MultipleExperimentInfos 
  {
  public:
    MultipleExperimentInfos();
    MultipleExperimentInfos(const MultipleExperimentInfos & other);
    virtual ~MultipleExperimentInfos();
    
    ExperimentInfo_sptr getExperimentInfo(const uint16_t runIndex);

    ExperimentInfo_const_sptr getExperimentInfo(const uint16_t runIndex) const;

    uint16_t addExperimentInfo(ExperimentInfo_sptr ei);

    void setExperimentInfo(const uint16_t runIndex, ExperimentInfo_sptr ei);

    uint16_t getNumExperimentInfo() const;

    void copyExperimentInfos(const MultipleExperimentInfos & other);
  protected:
    /// Returns a string description of the object
    const std::string toString() const;
  private:
    /// Vector for each ExperimentInfo class
    std::vector<ExperimentInfo_sptr> m_expInfos;

  };

  typedef boost::shared_ptr<MultipleExperimentInfos> MultipleExperimentInfos_sptr;
  typedef boost::shared_ptr<const MultipleExperimentInfos> MultipleExperimentInfos_const_sptr;

} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MULTIPLEEXPERIMENTINFOS_H_ */
