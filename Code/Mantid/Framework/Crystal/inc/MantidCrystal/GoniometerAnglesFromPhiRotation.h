/*
 * GoniometerAnglesFromPhiRotation.h
 *
 *  Created on: Apr 15, 2013
 *      Author: ruth
 */

#ifndef GoniometerAnglesFromPhiRotation_H_
#define GoniometerAnglesFromPhiRotation_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Logger.h"
#include "MantidDataObjects/PeaksWorkspace.h"



namespace Mantid
{
  namespace Crystal
  {
    class DLLExport GoniometerAnglesFromPhiRotation: public API::Algorithm
    {
    public:
      GoniometerAnglesFromPhiRotation();
      ~GoniometerAnglesFromPhiRotation();

      /// Algorithm's name for identification
      const std::string name() const
      {
        return "GoniometerAnglesFromPhiRotation";
      }
      ;

      /// Algorithm's version for identification
      int version() const
      {
        return 1;
      }
      ;

      /// Algorithm's category for identification
      const std::string category() const
      {
        return "Crystal";
      }

    private:

      /// Sets documentation strings for this algorithm
      void initDocs();

      /// Initialise the properties
      void init();
      Kernel::Matrix<double> getUBRaw(const Kernel::Matrix<double> &UB,
          const Kernel::Matrix<double>& GoniometerMatrix) const;

      bool CheckForOneRun(const DataObjects::PeaksWorkspace_sptr & Peaks,
          Kernel::Matrix<double> &GoniometerMatrix) const;

      void IndexRaw(const DataObjects::PeaksWorkspace_sptr &Peaks, const Kernel::Matrix<double> &UBraw, int &Nindexed,
          double &AvErrIndexed, double &AvErrorAll, double tolerance) const;

      /// Run the algorithm
      void exec();

      /// Static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } //Crystal
} //Mantid

#endif /* GoniometerAnglesFromPhiRotation_H_ */
