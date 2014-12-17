#include "MantidGPUAlgorithms/GPUTester.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include <CL/cl.hpp>
#include <string>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace GPUAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GPUTester)

// Declare the static members here.
bool GPUTester::m_kernelBuilt = false;
cl::Kernel GPUTester::kernel;
cl::CommandQueue GPUTester::queue;
cl::Context GPUTester::context;

GPUTester::

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    GPUTester::GPUTester() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
GPUTester::~GPUTester() {}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void GPUTester::init() {
  declareProperty("XSize", 256,
                  "X size of the data to give to the GPU card. Default 256");
  declareProperty("YSize", 256,
                  "Y size of the data to give to the GPU card. Default 256");
  declareProperty(
      new PropertyWithValue<bool>("Result", false, Direction::Output),
      "Result of the calculation. TRUE if successful.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void GPUTester::exec() {
  CPUTimer tim;
  if (!m_kernelBuilt)
    buildKernelFromFile("GPUTester_kernel.cl", "GPUTester_kernel", kernel,
                        queue, context);
  m_kernelBuilt = true;

  int iXSize = getProperty("XSize");
  int iYSize = getProperty("XSize");
  size_t XSize = size_t(iXSize);
  size_t YSize = size_t(iYSize);

  // Set Presistent memory only for AMD platform
  cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
  // TODO: For AMD platform only? Uses the shared memory in AMD-Vision to avoid
  // moving the memory around
  inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

  // -------------------- Create the input data -------------------------
  size_t numValues = (XSize * YSize);
  size_t bufferSize = numValues * sizeof(float);
  float *values = new float[numValues];
  for (size_t i = 0; i < numValues; i++)
    values[i] = float(i);

  /* Create memory object for input */
  cl::Buffer inputBuffer = cl::Buffer(context, inMemFlags, bufferSize,
                                      /* (void *) values*/ NULL, &err);
  checkError("Input Buffer creation failed.");

  /* ------------------Create memory object for output ---------------  */
  float *outputValues = new float[numValues];

  cl::Buffer outputBuffer =
      cl::Buffer(context, CL_MEM_WRITE_ONLY, bufferSize, NULL, &err);
  checkError("Output Buffer creation failed.");

  /* ------------------ Write memory from host to target ---------------  */
  cl::Event writeEvt;
  err = queue.enqueueWriteBuffer(
      inputBuffer, CL_FALSE, /* Non-blocking! dont modify the buffer */
      0,                     /* offset */
      bufferSize,            /* size */
      (void *)values,        /* Memory location in HOST */
      NULL, &writeEvt);
  checkError("queue.enqueueWriteBuffer() failed.");

  /* Set appropriate arguments to the kernel */

  /* input buffer image */
  err = kernel.setArg(0, inputBuffer);
  checkError("kernel.setArg(0) failed.");

  /* outBuffer imager */
  err = kernel.setArg(1, outputBuffer);
  checkError("kernel.setArg(1) failed.");

  // ---------------------------------------------------------------
  cl::NDRange globalThreads(XSize, YSize);
  cl::NDRange localThreads(32, 32);

  g_log.debug() << "Running CL program" << std::endl;
  err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalThreads,
                                   localThreads);
  checkError("CommandQueue::enqueueNDRangeKernel()");

  /* -------------------- Enqueue readBuffer ---------------------------*/
  cl::Event readEvt;
  err = queue.enqueueReadBuffer(outputBuffer, CL_FALSE, 0 /*offset*/,
                                bufferSize, outputValues, NULL, &readEvt);
  checkError("Queue.enqueueReadBuffer() failed.");

  std::cout << tim << " to set up the the commands" << std::endl;

  err = queue.finish();
  checkError("queue.finish() failed");
  std::cout << tim << " to run the OpenCL kernel" << std::endl;

  g_log.debug() << "OpenCL kernel execution compute." << std::endl;

  // We expect that the value = the position at each point
  bool result = true;
  for (size_t i = 0; i < numValues; i++) {
    if (outputValues[i] != float(i)) {
      result = false;
      break;
    }
  }

  if (result)
    g_log.notice() << "GPUTester runKernel succeeded - the output from the GPU "
                      "matched the expected values." << std::endl;
  else
    g_log.notice() << "GPUTester runKernel failed - the output from the GPU "
                      "did not match the expected values." << std::endl;

  // Set the output
  setProperty("Result", result);
}

} // namespace Mantid
} // namespace GPUAlgorithms
