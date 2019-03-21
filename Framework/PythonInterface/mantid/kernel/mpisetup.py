# For MPI builds we need to import boost.mpi to initialize the MPI environment.
# The environment is also instantiated in C++ but that does seem to lead to
# issues (probably related to how Ubuntu builds their MPI and Python packages).
if False:
    import boost.mpi
