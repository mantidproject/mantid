# Mantid Timers

This page provides an introduction to measuring execution time of the
Mantid C++ code.

```{contents}
:local:
```

## Wall-clock timers

Wall-clock time is the <span class="title-ref">total</span> time elapsed
during execution of code instructions. Mantid wall-clock timer classes
are based on the
[std::chrono::high_resolution_clock](https://en.cppreference.com/w/cpp/chrono/high_resolution_clock)
class.

### Class Timer

[MantidKernel/Timer](https://github.com/mantidproject/mantid/blob/main/Framework/Kernel/inc/MantidKernel/Timer.h)
works like a stop-watch. You need to instantiate it before the code
being profiled begins and then output its elapsed time right after the
code ends. You can also reset the timer between time measurements, if
necessary. An example is given below.

``` cpp
Mantid::Kernel::Timer timer;
// do something that takes 5 sec
std::cout << "did step 1 in " << timer << "\n";
// do something else that takes 55 sec - the timer still accumulates
std::cout << "did step 2 after " << timer << " from start\n";
timer.reset(); // reset the timer to zero
// do something else that takes 5 sec
std::cout << "step 3 took " << timer << "\n";

Output:
did step 1 in 5 sec
did step 2 after 60 sec
step 3 took 5 sec
```

### Class CodeBlockTimer

[MantidKernel/CodeBlockTimer](https://github.com/mantidproject/mantid/blob/main/Framework/Kernel/inc/MantidKernel/Timer.h)
is designed for timing a block of C++ code inside curly braces, i.e.
inside a scope. You just need to instantiate a timer object on the stack
at the start of the scope and provide an output stream. When the timer
goes out of scope, it automatically outputs the elapsed time to the
stream. An example is given below.

``` cpp
#include "Timer.h"
void MyAlgorithm::doSomething()
{
    // Instantiate a timer object on the stack. Specify the name and output stream.
    Mantid::Kernel::CodeBlockTimer timer("MyAlgorithm::doSomething", std::cout);
    // Do something that takes 5 sec
    // ...
    //
    // When the timer goes out of scope, it will output the results
}

Output: Elapsed time (sec) in "MyAlgorithm::doSomething": 5
```

The above example shows how to time a whole function. You can, however,
break a function into smaller blocks of code using curly braces and time
each code block separately. Note, when a code block is executed multiple
times, <span class="title-ref">CodeBlockTimer</span> will output the
elapsed time for each execution. If you don't want to have multiple
lines of output and are interested in the total elapsed time, use
`CodeBlockMultipleTimer <CodeBlockMultipleTimer>`.

### Class CodeBlockMultipleTimer

[MantidKernel/CodeBlockMultipleTimer](https://github.com/mantidproject/mantid/blob/main/Framework/Kernel/inc/MantidKernel/Timer.h)
is designed for timing a block of code that is called multiple times.
Similar to `CodeBlockTimer <CodeBlockTimer>`, it needs to be
instantiated on the stack at the start of the scope. The constructor
takes in a
[MantidKernel/CodeBlockMultipleTimer::TimeAccumulator](https://github.com/mantidproject/mantid/blob/main/Framework/Kernel/inc/MantidKernel/Timer.h)
object. Unlike the timer, which gets destroyed at the end of the scope,
the time accumulator has to be persistent. One possibility is to declare
it with a static linkage in the same compilation unit and provide an
accessor function. Every time the timer goes out of scope, it
automatically updates the time accumulator. The time accumulator can
later be accessed to report the total elapsed time. In the example below
Algorithm B calls a method on Algorithm A several times and outputs the
total time elapsed in that method.

``` cpp
// File MyAlgorithm_A.cpp
#include "Timer.h"
// Declare a time accumulator with a static linkage
static Mantid::Kernel::CodeBlockMultipleTimer::TimeAccumulator s_timeAccumulator("MyAlgorithm_A::doSomething");
// Provide an accessor function
MANTID_KERNEL_DLL const Mantid::Kernel::CodeBlockMultipleTimer::TimeAccumulator& myAlgorithm_A_TimeAccumulator(){
    return s_timeAccumulator;
}
// ...
void MyAlgorithm_A::doSomething(){
    // Declare a timer object on the stack. Specify the time accumulator.
    Mantid::Kernel::CodeBlockMultipleTimer timer(s_timeAccumulator);
    // Do something that takes 5 sec
    // ...
    //
    // When the timer goes out of scope, it will update the time accumulator
}

// File MyAlgorithm_B.cpp
#include "Timer.h"
// Let the linker know that the accessor function for the time accumulator is defined in another compilation unit
extern const Mantid::Kernel::CodeBlockMultipleTimer::TimeAccumulator& myAlgorithm_A_TimeAccumulator();

void MyAlgorithm_B::doSomething(){
    // Call MyAlgorithm_A::doSomething() 3 times
    // ...
    //
    // Output the results
    std::cout << myAlgorithm_A_TimeAccumulator() << '\n';
}

Output: Elapsed time (sec) in "MyAlgorithm_A::doSomething": 15; Number of entrances: 3
```

## CPU timers

CPU time is the time spent by the CPU while processing code
instructions. Unlike wall-clock time, CPU time does not include time
spent waiting for disk, network or other resources, e.g. I/O operations.

### Class CPUTimer

[MantidKernel/CPUTimer](https://github.com/mantidproject/mantid/blob/main/Framework/Kernel/inc/MantidKernel/CPUTimer.h)
measures both CPU time and wall-clock time. The CPU time measurement
utilizes
[std::clock()](https://en.cppreference.com/w/cpp/chrono/c/clock)
function. The wall-clock time measurement uses `Timer <Timer>` class.
The output includes the wall-clock time and the ratio of the CPU time to
the wall-clock time. Note, since CPU time and wall-clock time are
measured with different accuracy, it is possible to have a ratio greater
than 1. A code example is given below.

``` cpp
Mantid::Kernel::CPUTimer timer;
// do something that takes 5 sec
std::cout << "did step 1 in " << timer << "\n";
// do something else that takes 55 sec - the timer still accumulates
std::cout << "did step 2 after " << timer << " from start\n";
timer.reset(); // reset the timer to zero
// do something else that takes 5 sec
std::cout << "step 3 took " << timer << "\n";

Output:
did step 1 in  5.0000 sec, CPU Fraction 1.00
did step 2 after 60.0000 sec, CPU Fraction 0.99
step 3 took  5.0000 sec, CPU Fraction 1.00
```
