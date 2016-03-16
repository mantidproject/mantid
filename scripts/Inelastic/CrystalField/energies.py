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

              
def energies(nre, B20, B22, B40, B42, B44):
      """
      Calculate the crystal field energies and wavefunctions.
      
      Args:
            nre: a number denoting a rare earth ion
            B20, B22, B40, B42, B44: values of the crystal field parameters.
            
      Return:
            a tuple of energies(1D numpy array) and wavefunctions (2D numpy array)
      """
      # Do the calculations
      res = CrystalFieldEnergies(nre, B20=B20, B22=B22, B40=B40, B42=B42, B44=B44)

      # Unpack the results
      energies = res[0]
      n = len(energies)
      wavefunctions = _unpack_complex_matrix(res[1], n, n)
      hamiltonian = _unpack_complex_matrix(res[2], n, n)
              
      return energies, wavefunctions, hamiltonian
