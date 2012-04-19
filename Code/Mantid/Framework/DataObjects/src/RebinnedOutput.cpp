#include "MantidDataObjects/RebinnedOutput.h"

#include "MantidAPI/WorkspaceFactory.h"

#include <algorithm>
#include <iostream>

//using namespace Mantid::Kernel;
//using namespace Mantid::API;

namespace Mantid
{
namespace DataObjects
{
  DECLARE_WORKSPACE(RebinnedOutput)

  // Get a reference to the logger
  Kernel::Logger& RebinnedOutput::g_log = Kernel::Logger::get("RebinnedOutput");

  RebinnedOutput::RebinnedOutput() : Workspace2D()
  {
  }
    
  RebinnedOutput::~RebinnedOutput()
  {
    /*
    // Clear out the memory
    for (std::size_t i = 0; i < this->fracArea.size(); i++)
    {
      delete this->fracArea[i];
    }
    */
  }
  
  /**
   * Gets the name of the workspace type.
   * @return Standard string name
   */
  const std::string RebinnedOutput::id() const
  {
    return "RebinnedOutput";
  }

  /**
   * Sets the size of the workspace and initializes arrays to zero
   * @param NVectors :: The number of vectors/histograms/detectors in the workspace
   * @param XLength :: The number of X data points/bin boundaries in each vector (must all be the same)
   * @param YLength :: The number of data/error points in each vector (must all be the same)
   */
  void RebinnedOutput::init(const std::size_t &NVectors,
                            const std::size_t &XLength,
                            const std::size_t &YLength)
  {
    Workspace2D::init(NVectors, XLength, YLength);
    std::size_t nHist = this->getNumberHistograms();
    this->fracArea.resize(nHist);
    for (std::size_t i = 0; i < nHist; ++i)
    {
      this->fracArea[i].resize(YLength);
    }
  }

  /**
   * Function that returns a fractional area array for a given index.
   * @param index :: the array to fetch
   * @return the requested fractional area array
   */
  MantidVec &RebinnedOutput::dataF(const std::size_t index)
  {
    return this->fracArea[index];
  }

  /**
   * Function that returns a fractional area array for a given index. This
   * returns an unmodifiable array.
   * @param index :: the array to fetch
   * @return the requested fractional area array
   */
  const MantidVec &RebinnedOutput::dataF(const std::size_t index) const
  {
    return this->fracArea[index];
  }

  /**
   * This function takes the data/error arrays and divides them by the
   * corresponding fractional area array. This creates a representation that
   * is easily visualized. The Rebin and Integration algorithms will have to
   * undo this in order to properly treat the data.
   */
  void RebinnedOutput::finalize()
  {
    g_log.information() << "Starting finalize procedure." << std::endl;
    std::size_t nHist = this->getNumberHistograms();
    g_log.information() << "Number of histograms: " << nHist << std::endl;
    for (std::size_t i = 0; i < nHist; ++i)
    {
      MantidVec &data = this->dataY(i);
      MantidVec &err = this->dataE(i);
      MantidVec &frac = this->dataF(i);
      MantidVec frac_sqr(frac.size());

      std::transform(data.begin(), data.end(), frac.begin(), data.begin(),
                     std::divides<double>());
      std::transform(frac.begin(), frac.end(), frac.begin(), frac_sqr.begin(),
                     std::multiplies<double>());
      std::transform(err.begin(), err.end(), frac_sqr.begin(), err.begin(),
                     std::divides<double>());
    }
  }

} // namespace Mantid
} // namespace DataObjects
