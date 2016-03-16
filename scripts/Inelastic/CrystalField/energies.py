from mantid.simpleapi import CrystalFieldEnergies
import numpy as np

def _unpack_complex_matrix(packed, n1, n2):
      unpacked = np.ndarray(n1*n2, dtype=complex).reshape((n1, n2))
      for i in range(n1):
          for j in range(n2):
              k = 2 * (i*n2 + j)
              value = complex(packed[k], packed[k+1])
              unpacked[i,j] = value
      return unpacked

              
def energies(nre, **kwargs):
      """
      Calculate the crystal field energies and wavefunctions.

      Args:
            nre: a number denoting a rare earth ion
            |1=Ce|2=Pr|3=Nd|4=Pm|5=Sm|6=Eu|7=Gd|8=Tb|9=Dy|10=Ho|11=Er|12=Tm|13=Yb|

            kwargs: the keyword arguments for crystal field parameters.
                    They can be:
                        B20, B22, B40, B42, B44, ... : real parts of the crystal field parameters.
                        IB20, IB22, IB40, IB42, IB44, ... : imaginary parts of the crystal field parameters.
                        Bmol: a list of 3 molecular field parameters
                        Bext: a list of 3 external field parameters
            
      Return:
            a tuple of energies(1D numpy array), wavefunctions (2D numpy array)
            and the hamiltonian (2D numpy array).
      """
      # Do the calculations
      res = CrystalFieldEnergies(nre, **kwargs)

      # Unpack the results
      energies = res[0]
      n = len(energies)
      wavefunctions = _unpack_complex_matrix(res[1], n, n)
      hamiltonian = _unpack_complex_matrix(res[2], n, n)
              
      return energies, wavefunctions, hamiltonian
