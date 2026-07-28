- The dimensions of an :class:`~mantid.api.IMDHistoWorkspace` can now have their ``name`` and ``units`` edited from
  Python via ``getDimension(i).setName(...)`` and ``getDimension(i).setUnits(...)``. Editing units is supported for
  HKL and general frames; an HKL dimension accepts ``r.l.u.`` or an inverse-Angstrom-style label such as
  ``in 2.5 A^-1``. This is useful for relabelling the dimensions produced by ``MDNorm`` for unusual projections.
