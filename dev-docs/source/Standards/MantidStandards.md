# Mantid Standards

These standards relate specifically to the implementation of the Mantid
framework and the workbench application.

```{contents}
:local:
```

## General Notes on Naming

The items described on this page form part of the public interface to
Mantid, which means that their names are hard to change once they are
out in the wild. As naming is one of the most difficult tasks to [do
well](http://martinfowler.com/bliki/TwoHardThings.html), it is important
to take time and care when choosing a name. Some guidance on naming in a
more code-related context can be found
[here](http://blog.codinghorror.com/i-shall-call-it-somethingmanager/)
but the advice should still be applicable to naming Mantid concepts.

## Algorithms

Standards to follow when implementing an
[algorithm](http://docs.mantidproject.org/nightly/concepts/Algorithm.html)
in both C++ and Python.

### Naming

As a general purpose data process platform for global users, the names
of the Mantid algorithms need to be as self evident as possible so that
users can intuitively understand the functionality and limitations of
the algorithms they are using. To this end, it is important for Mantid
developers to use clear and concise names during the renaming process.
Generally speaking, the name of a algorithm can contain up to **four**
sections:

```{admonition} Mantid Algorithm Naming Convention
\[Technique\] \[Instrument/Facility\] Action Target
```

- Technique

  If given algorithm is designed for a specific technique, the algorithm
  name should start with the abbreviation of the technique. However,
  this section can be omitted if the algorithm is not technique specific
  (such as file loading and data plotting). Here are some commonly used
  abbreviations of techniques

  <table>
  <thead>
  <tr class="header">
  <th>Abbreviations</th>
  <th>Full Description</th>
  </tr>
  </thead>
  <tbody>
  <tr class="odd">
  <td><strong>CWPD</strong></td>
  <td><blockquote>
  <p>Constant Wavelength Powder Diffraction</p>
  </blockquote></td>
  </tr>
  <tr class="even">
  <td><strong>PD</strong></td>
  <td><blockquote>
  <p>Powder Diffraction</p>
  </blockquote></td>
  </tr>
  <tr class="odd">
  <td><strong>REFL</strong></td>
  <td><blockquote>
  <p>Reflectometry</p>
  </blockquote></td>
  </tr>
  <tr class="even">
  <td><strong>SANS</strong></td>
  <td><blockquote>
  <p>Small Angle Neutron Scattering</p>
  </blockquote></td>
  </tr>
  <tr class="odd">
  <td><strong>SCD</strong></td>
  <td><blockquote>
  <p>Single Crystal Diffraction</p>
  </blockquote></td>
  </tr>
  </tbody>
  </table>

  ```{note}
  The table above is a work in progress as more abbreviations will be
  added in the future.
  ```

- Instrument/Facility

  As Mantid is a collaboration across many different institutes, it is
  very common to have some algorithms that are specifically designed for
  a special instrument or a facility. For algorithms like these, it is
  important to have the abbreviations of the corresponding instrument or
  facility clearly shown in the name. On the other hand, this section
  can be skipped if the algorithm is general enough that its application
  is no longer tied to a specific instrument or facility.

  - Here are some commonly used abbreviations of facilities

  <table>
  <thead>
  <tr class="header">
  <th>Abbreviations</th>
  <th>Full Description</th>
  </tr>
  </thead>
  <tbody>
  <tr class="odd">
  <td><strong>ILL</strong></td>
  <td><blockquote>
  <p>Institut Laue-Langevin at GRENOBLE,France</p>
  </blockquote></td>
  </tr>
  <tr class="even">
  <td><strong>ISIS</strong></td>
  <td><blockquote>
  <p>ISIS Neutron and Muon Source at UK</p>
  </blockquote></td>
  </tr>
  <tr class="odd">
  <td><strong>HFIR</strong></td>
  <td><blockquote>
  <p>High Flux Isotope Reactor at ORNL,USA</p>
  </blockquote></td>
  </tr>
  <tr class="even">
  <td><strong>SNS</strong></td>
  <td><blockquote>
  <p>Spallation Neutron Source at ORNL,USA</p>
  </blockquote></td>
  </tr>
  </tbody>
  </table>

  ```{note}
  The table above is a work in progress as more abbreviations will be
  added in the future.
  ```

  - Here are some commonly used abbreviations of instruments

  | Abbreviations | Full Description                                     |
  |---------------|------------------------------------------------------|
  | **CORELLI**   | Elastic Diffuse Scattering Spectrometer at BL-9, SNS |
  | **POWGEN**    | Powder Diffractometer at BL-11A, SNS                 |
  | **TOPAZ**     | Single-Crystal Diffractometer at BL-12, SNS          |
  | **WAND2**     | Wide-Angle Neutron Diffractometer at HB-2C, HFIR     |

  ::: {.note}
  ::: {.title}
  Note
  :::

  The table above is a work in progress as more abbreviations will be
  added in the future.
  :::

- Action

  As data process platform, Mantid perform various action via
  algorithms, therefore it is crucial to have clear and concise
  description of intended action depicted in the algorithm name.

- Target

  Most of the time the action term above requires a specific receiving
  end, namely a target. Depending on the action, sometimes the target
  can be omitted if it is self evident (such as `LoadFiles` can be
  simplified into `Load`).
``` {.admonition}
Example

`SCDCalibratePanels` indicates this is a algorithm designed for single
crystal diffraction technique that is not tied to a specific instrument
or facility. It performs calibration of panel type detectors.
```

Algorithm names start with a capital letter and have a capital letter
for each new word, with no underscores. Use alphabet and numeric
characters only. Numbers are only allowed after the first character.

Names should be descriptive but not too long (max 20 chars). If
possible, avoid abbreviations unless they are common and well understood
(see examples above). To avoid a proliferation of different synonyms for
algorithms that have a common goal, e.g. Create... vs Generate..., we
standardise on a set of prefixes for common tasks:

| Task                                                                  | Preferred Prefix | Example            |
|-----------------------------------------------------------------------|------------------|--------------------|
| Creating a new object, e.g. workspace, with exception of file loaders | Create           | CreateMDWorkspace  |
| Loading a file                                                        | Load             | LoadMappingTable   |
| Applying a value to an existing object, e.g set UB matrix             | Set              | SetUB              |
| Retrieve a value, e.g. Ei                                             | Get              | GetDetectorOffsets |
| Adding a new item to an existing list                                 | Add              | AddSampleLog       |
| Search for something, e.g. peaks                                      | Find             | FindPeaks          |

### Categories

Plain english using [Title
Case](http://www.grammar-monster.com/lessons/capital_letters_title_case.htm).
Connecting words should have lower case first letters. Use alphabet
characters only, numbers are not allowed, e.g. Muon or SANS.

### Properties

Property names start with a capital letter and have a capital letter for
each new word, with no underscores. Use alphabet and numeric characters
only. Numbers are only allowed after the first character.

Wherever possible and unambiguous, the primary input workspace should be
called `InputWorkspace` and the primary output workspace should be
called `OutputWorkspace`. An algorithm with a single In/Out workspace
should name its property `Workspace`. Certain groups of algorithms have
other standards to adhere to.

## Fit Functions

Standards to following when implementing a fitting function (both C++ &
Python).

### Naming

Function names start with a capital letter and have a capital letter for
each new word, with no underscores. Use alphabet and numeric characters
only. Numbers are only allowed after the first character.

### Categories

Plain english using [Title
Case](http://www.grammar-monster.com/lessons/capital_letters_title_case.htm).
Connecting words should have lower case first letters. Numbers are not
allowed.

### Parameters

Parameter names must:

- Start with a capital letter
- Have a capital letter for each new word (e.g. 'InputWorkspace')
- Use alphanumeric characters only (i.e. cannot contain any of these
  `/,._-'"` or whitespace)
- Can contain numbers but only allowed after the first character.

Notable exceptions to these rules are lattice constants (i.e. a, b, c,
alpha, beta, gamma).

## Workspace Names

No firm restrictions. The use of two underscores as a prefix will mark
the workspace as hidden. It is recommended to use only the alphabet,
numeric and the underscore characters.
