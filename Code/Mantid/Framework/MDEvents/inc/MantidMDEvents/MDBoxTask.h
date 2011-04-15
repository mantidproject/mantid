#ifndef MANTID_MDEVENTS_MDBOXTASK_H_
#define MANTID_MDEVENTS_MDBOXTASK_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/Task.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDBox.h"


namespace Mantid
{
namespace MDEvents
{

  /** Abstract class for a task to be run inside the hierarchy of MDGridBoxes.
   * EXPERIMENTAL...
   * 
   * @author
   * @date 2011-04-13 18:35:18.453468
   */
  TMDE_CLASS
  class DLLExport MDBoxTask : public Kernel::Task
  {
  public:
    MDBoxTask(IMDBox<MDE,nd> * inBox);
    ~MDBoxTask();
    

    /** Run the task from the ThreadPool */
    void run()
    {
      // Start off the run.
      // The MDGridBox will call evaluateMDBox() and/or boxContained.
      inBox->runMDBoxTask(this);
    }


    /** Return true if the task should stop going deeper when
     * it reaches a MDGrixBox that is fully contained.
     * It then calls evaluateMDGridBox(), which then presumably uses
     * the cached integrated signal.
     */
    virtual bool stopOnFullyContained()
    {
      return true;
    }


    /** Is the box given contained in the volume of interest
     * of this task?
     *
     * @param coords :: nd-sized array of the coordinates
     * @return  0 if not contained;
     *          1 if partially contained
     *          2 if fully contained.
     */
    virtual int boxContained(IMDBox<MDE,nd> * box)
    {
      return 0;
    }

    /** Evaluate the contents of a un-split MDBox.
     *
     * @param box :: reference to the MDBox
     * @param fullyContained :: true if the box was found to be fully contained
     *        within the volume of interest. It can then use the cached
     *        signal, for example.
     */
    virtual void evaluateMDBox(MDBox<MDE,nd> * box, bool fullyContained) = 0;

    /** Evaluate the contents of a MDGridBox that is fully contained within the
     * volume of interest.
     *
     * This will only be called if stopOnFullyContained() returns true.
     *
     * @param box :: reference to the MDGridBox
     */
//    virtual void evaluateMDGridBox(MDGridBox<MDE,nd> * box) = 0;

  protected:
    /// pointer to the MDBox that is the starting point of the execution
    IMDBox<MDE,nd> * inBox;


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDBOXTASK_H_ */
