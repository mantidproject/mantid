// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LoadHelper_H_
#define MANTID_DATAHANDLING_LoadHelper_H_

#include "MantidAPI/Run.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {

namespace Kernel {
class Quat;
}

namespace DataHandling {

/** LoadHelper : Auxiliary File for Loading Files
 */
class DLLExport LoadHelper {
public:
  virtual ~LoadHelper() = default;

  std::string findInstrumentNexusPath(const Mantid::NeXus::NXEntry &);
  std::string getStringFromNexusPath(const Mantid::NeXus::NXEntry &,
                                     const std::string &);
  double getDoubleFromNexusPath(const Mantid::NeXus::NXEntry &,
                                const std::string &);
  std::vector<double>
  getTimeBinningFromNexusPath(const Mantid::NeXus::NXEntry &,
                              const std::string &);
  static double calculateStandardError(double in) { return sqrt(in); }
  double calculateEnergy(double);
  double calculateTOF(double, double);
  double getInstrumentProperty(const API::MatrixWorkspace_sptr &, std::string);
  void addNexusFieldsToWsRun(NXhandle nxfileID, API::Run &runDetails);
  void dumpNexusAttributes(NXhandle nxfileID, std::string &indentStr);
  std::string dateTimeInIsoFormat(std::string);

  void moveComponent(API::MatrixWorkspace_sptr ws,
                     const std::string &componentName,
                     const Kernel::V3D &newPos);
  void rotateComponent(API::MatrixWorkspace_sptr ws,
                       const std::string &componentName,
                       const Kernel::Quat &rot);
  Kernel::V3D getComponentPosition(API::MatrixWorkspace_sptr ws,
                                   const std::string &componentName);

private:
  void recurseAndAddNexusFieldsToWsRun(NXhandle nxfileID, API::Run &runDetails,
                                       std::string &parent_name,
                                       std::string &parent_class, int level);
};
} // namespace DataHandling
// namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LoadHelper_H_ */
