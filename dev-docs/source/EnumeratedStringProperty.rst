.. _EnumeratedStringProperty:

What is EnumeratedStringProperty?
---------------------------------

``EnumeratedStringProperty`` allows the use of ``EnumeratedString`` objects within the Property structure framework.

How to use the EnumeratedStringProperty
---------------------------------------

Include the ``EnumeratedStringProperty.h`` header file. Set up the ``EnumeratedStringProperty`` as follows:

.. code-block:: cpp

    namespace {
        const std::vector<std::string> binningModeNames{"Default", "Linear", "Logarithmic", "ReverseLogarithmic", "Power"};
        enum class BinningMode { DEFAULT, LINEAR, LOGARITHMIC, REVERSELOG, POWER, enum_count };
        typedef Mantid::Kernel::EnumeratedString<BinningMode, &binningModeNames> BINMODE;
    } // namespace

Declare property:

.. code-block:: cpp

    declareProperty(
        std::make_unique<EnumeratedStringProperty<BinningMode, &binningModeNames>>("PropertyName"),
        "Description"
    );

Use declared property:

.. code-block:: cpp

    BINMODE binMode = someName;
    if (binMode == BinningMode::LINEAR)
        do_something();
    else if (binMode != "Default")
        do_something_else();

Determining and using pre-set mode, if present, or using the default setting:

.. code-block:: cpp

    BINMODE binMode;
    if (existsProperty("PropertyName"))
        Mode = getPropertyValue("PropertyName");
    else
        Mode = "Default";

Example Use of EnumeratedString
-------------------------------

Please see examples of usage in ``Rebin.cpp``, ``CalculateDIFC.cpp``, and ``AddSampleLog.cpp``.
