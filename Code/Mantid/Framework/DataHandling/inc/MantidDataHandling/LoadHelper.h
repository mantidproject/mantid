#ifndef MANTID_DATAHANDLING_LoadHelper_H_
#define MANTID_DATAHANDLING_LoadHelper_H_

#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid
{
  namespace DataHandling
  {

    /** LoadHelper : Auxiliary File for Loading Files

     Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport LoadHelper
    {
    public:

      LoadHelper();
      virtual ~LoadHelper();

      std::string findInstrumentNexusPath(const Mantid::NeXus::NXEntry&);
      std::string getStringFromNexusPath(const Mantid::NeXus::NXEntry&, const std::string&);
      double getDoubleFromNexusPath(const Mantid::NeXus::NXEntry&, const std::string&);
      std::vector<double> getTimeBinningFromNexusPath(const Mantid::NeXus::NXEntry &,
          const std::string &);
      static double calculateStandardError(double in)
      {
        return sqrt(in);
      }
      double calculateEnergy(double);
      double calculateTOF(double, double);
      double getL1(const API::MatrixWorkspace_sptr&);
      double getL2(const API::MatrixWorkspace_sptr&, int detId = 1);
      double getInstrumentProperty(const API::MatrixWorkspace_sptr&, std::string);
      void addNexusFieldsToWsRun(NXhandle nxfileID, API::Run& runDetails);
      void dumpNexusAttributes(NXhandle nxfileID, std::string& indentStr);
      std::string dateTimeInIsoFormat(std::string);

      void moveComponent(API::MatrixWorkspace_sptr ws, const std::string &componentName,
          const Kernel::V3D& newPos);
      void rotateComponent(API::MatrixWorkspace_sptr ws, const std::string &componentName,
          const Kernel::Quat & rot);
      Kernel::V3D getComponentPosition(API::MatrixWorkspace_sptr ws, const std::string &componentName);
      template<typename T>
      T getPropertyFromRun(API::MatrixWorkspace_const_sptr inputWS, const std::string& propertyName);

    private:
      void recurseAndAddNexusFieldsToWsRun(NXhandle nxfileID, API::Run& runDetails,
          std::string& parent_name, std::string& parent_class, int indent);

    };

  }
// namespace DataHandling
}// namespace Mantid

#endif  /* MANTID_DATAHANDLING_LoadHelper_H_ */
