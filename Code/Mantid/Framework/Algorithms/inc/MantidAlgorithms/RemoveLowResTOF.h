#ifndef REMOVELOWRESTOF_H_
//includes
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
class DLLExport RemoveLowResTOF : public API::Algorithm
{
public:
  RemoveLowResTOF();
  virtual ~RemoveLowResTOF();
  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
  void execEvent();

  API::MatrixWorkspace_const_sptr m_inputWS; ///< Pointer to the input workspace
  DataObjects::EventWorkspace_const_sptr m_inputEvWS; ///< Pointer to the input event workspace
  double calcTofMin(const std::size_t);
  double m_DIFCref; ///< The reference value for DIFC to filter with
  double m_K; ///< Mystery variable that I'm not sure what it is for
  Geometry::IInstrument_const_sptr m_instrument; //< The instrument
  double m_L1; ///< The instrument initial flightpath
  double m_Tmin; ///< The start of the time-of-flight frame
  std::size_t m_numberOfSpectra; ///< The number of spectra in the workspace
  API::Progress* m_progress;   ///< Progress reporting
};

} // namespace Algorithms
} // namespace Mantid

#define REMOVELOWRESTOF_H_


#endif /* REMOVELOWRESTOF_H_ */
