
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculate the factor(s) that best match the individual spectra to a
reference spectrum using `Gauss-Markov process
<https://en.wikipedia.org/wiki/Gauss%E2%80%93Markov_process>`_. This
is the same algorithm used by the "blend" step of `PDFgetN
<http://pdfgetn.sourceforge.net/>`_. The data undergoes the linear
transformation

.. math::

   scale * y_{spectrum} + offset

such that the distance between :math:`y_{reference}` and the
transformed spectrum is minimized. Only the portions of the spectra
where the spectra overlap with the same x-values and the uncertainties
are greater than zero are considered.

.. note::

   Gauss-Markov does not take into account varying uncertainties when
   calculating the ``scale`` and ``offset``.

Usage
-----

.. testcode:: MatchSpectra

    import numpy as np
    x = np.arange(100, dtype=float)
    x = np.tile(x, 2)
    y = np.arange(100, dtype=float)
    y = np.tile(y, 2)
    y[100:200] += 10
    dy = np.zeros(y.size, dtype=float) + 1.
    CreateWorkspace(OutputWorkspace='MatchSpectra_input',
                    DataX=x,
                    DataY=y,
                    DataE=dy,
                    NSpec=2)
    _, offset, scale, chisq = MatchSpectra(InputWorkspace='MatchSpectra_input',
                                           OutputWorkspace='MatchSpectra_output',
                                           ReferenceSpectrum=2,
                                           CalculateOffset=True,
                                           CalculateScale=False)
    for i in range(2):
        print('spectra {}: {:.1f} * y + {:.1f}, chisq={:.1f}'.format(i+1, scale[i], offset[i], chisq[i]))

.. testcleanup:: MatchSpectra

   DeleteWorkspaces(['MatchSpectra_input','MatchSpectra_output'])


Output:

.. testoutput:: MatchSpectra

    spectra 1: 1.0 * y + 10.0, chisq=0.0
    spectra 2: 1.0 * y + 0.0, chisq=0.0

.. categories::

.. sourcelink::
