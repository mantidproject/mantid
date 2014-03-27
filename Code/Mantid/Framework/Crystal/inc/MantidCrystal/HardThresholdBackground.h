#ifndef MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUND_H_
#define MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUND_H_

#include "MantidKernel/System.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
namespace Crystal
{

  /** HardThresholdBackground : Implementation of BackgroundStrategy using a fixed background signal value as the threshold.
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport HardThresholdBackground : public BackgroundStrategy
  {
  public:
    HardThresholdBackground(const double thresholdSignal, const Mantid::API::MDNormalization normalisation);
    virtual bool isBackground(Mantid::API::IMDIterator* iterator) const;
    void configureIterator(Mantid::API::IMDIterator* const iterator) const;
    virtual ~HardThresholdBackground();
  private:
    /// Cutoff
    const double m_thresholdSignal;
    /// Normalization
    const Mantid::API::MDNormalization m_normalization;
  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUND_H_ */
