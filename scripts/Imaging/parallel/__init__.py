"""
parallel.shared_mem: Runs a function in parallel. Expects and uses a 3D shared memory array between the processes.
parallel.exclusive_mem: Runs a function in parallel. Uses a 3D memory array, but each process will copy the piece of 
                        data before processing it, if the data is in any way read or modified, triggering the copy-on-write.
"""
