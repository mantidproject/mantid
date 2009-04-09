#ifndef MANTID_KERNEL_MULTITHREADED_H_
#define MANTID_KERNEL_MULTITHREADED_H_

// _OPENMP is automatically defined if openMP support is enabled in the compiler.
#ifdef _OPENMP

// The syntax used to dfine a pragma within a macro is different on windows and GCC
#ifdef _MSC_VER 
#define PRAGMA __pragma
#else //_MSC_VER 
#define PRAGMA _Pragma
#endif //_MSC_VER 

#include <omp.h>

/** Includes code to add OpenMP commands to run the next for loop in parallel.
*		This includes no checks to see if workspaces are suitable 
*		and therefore should not be used in any loops that access workspaces.
*/
#define PARALLEL_FOR_NO_WSP_CHECK() \
		PRAGMA(omp parallel for)

/** Includes code to add OpenMP commands to run the next for loop in parallel.
*		The workspace is checked to ensure it is suitable for multithreaded access.
*/
#define PARALLEL_FOR1(workspace1) \
		PRAGMA(omp parallel for if (workspace1->threadSafe()))

/** Includes code to add OpenMP commands to run the next for loop in parallel.
*	 Both workspaces are checked to ensure they suitable for multithreaded access.
*/
#define PARALLEL_FOR2(workspace1, workspace2) \
		PRAGMA(omp parallel for if (workspace1->threadSafe() && workspace2->threadSafe()))

/** Includes code to add OpenMP commands to run the next for loop in parallel.
*	 All three workspaces are checked to ensure they suitable for multithreaded access.
*/
#define PARALLEL_FOR3(workspace1, workspace2, workspace3) \
		PRAGMA(omp parallel for if (workspace1->threadSafe() && workspace2->threadSafe() && workspace3->threadSafe()))

#else //_OPENMP
///Empty definitions - to enable set your complier to enable openMP
#define PARALLEL_FOR_NO_WSP_CHECK()
#define PARALLEL_FOR1(workspace1)
#define PARALLEL_FOR2(workspace1, workspace2)
#define PARALLEL_FOR3(workspace1, workspace2, workspace3)
#endif //_OPENMP

#endif //MANTID_KERNEL_MULTITHREADED_H_