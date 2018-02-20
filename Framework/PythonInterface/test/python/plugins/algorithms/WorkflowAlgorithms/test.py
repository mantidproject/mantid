import numpy as np

def _normalize_by_index(workspace, index):
    """
    Normalize each spectra of the specified workspace by the y-value at the
    specified index in that spectra, while accounting for errors on those values.
    @param workspace    The workspace to normalize.
    @param index        The index of the y-value to normalize by.
    """
    y_vals = workspace.extractY()
    y_vals_e = workspace.extractE()
    scale = y_vals[:,index]
    scale_e = y_vals_e[:,index]
    np.reciprocal(scale, out=scale)
    np.einsum('ij,i->ij', y_vals, scale, out=y_vals)
    # error propagation: C = A / B ; dC = sqrt( (dA/B)^2 + (A*dB/B^2)^2 ) ||
    # !! wrong for A=B (index by which is scaled = index) !!
    a = np.einsum('ij,i->ij', y_vals_e, scale)
    b = np.einsum('ij,i->ij', y_vals, scale_e * scale)
    np.sqrt(a ** 2 + b ** 2, out=y_vals_e)
    y_vals_e[index] = a[index]
    workspace.setY(y_vals)
    workspace.setE(y_vals_e)

