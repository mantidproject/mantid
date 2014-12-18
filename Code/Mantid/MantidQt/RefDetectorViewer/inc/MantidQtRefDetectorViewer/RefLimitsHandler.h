#ifndef REF_LIMITS_HANDLER_H
#define REF_LIMITS_HANDLER_H

#include "ui_RefImageView.h"

namespace MantidQt
{
namespace RefDetectorViewer
{
/** Retrieves the states of the peak/background/TOF limit settings from the gui

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

    Code Documentation is available at <http://doxygen.mantidproject.org>
 */

class RefLimitsHandler
{
public:
  /// Construct object to manage range (peak/back/TOF) controls in the UI
  RefLimitsHandler( Ui_RefImageViewer* ivUI );

  /// get peak, back and tof values
  int getPeakLeft() const;
  int getPeakRight() const;
  int getBackLeft() const;
  int getBackRight() const;
  int getTOFmin() const;
  int getTOFmax() const;

  void setPeakLeft(const int value);
  void setPeakRight(const int value);
  void setBackLeft(const int value);
  void setBackRight(const int value);
  void setTOFmin(const int value);
  void setTOFmax(const int value);

  void setActiveValue(const double x, const double y);

private:
  const Ui_RefImageViewer* const m_ui;

};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_LIMITS_HANDLER_H
