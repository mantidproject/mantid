/***************************************************************************/
/**
 This file is a SIP file for binding types used in Qwt libraries.
 This is not a stand-alone SIP file, it must be "%Include"d into
 another
*/
/***************************************************************************/


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/




%MappedType QwtDoubleInterval
{
%TypeHeaderCode
// Include the library interface to the type being mapped.
#include <qwt_double_interval.h>
%End

%ConvertToTypeCode
    // See if we are just being asked to check the type of the Python
    // object.
    if (sipIsErr == NULL)
    {
        // Check it is a list.
        if (!PyList_Check(sipPy))
            return 0;

//        // Now check each element of the list is a double
//        for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
//            if (!sipCanConvertToType(PyList_GET_ITEM(sipPy, i),
//                                     sipType_double, 0))
//                return 0;

		// List must be of size 2, otherwise it does not work
		if (PyList_GET_SIZE(sipPy) != 2)
                return 0;
        return 1;
    }

    // Create the instance on the heap.
    PyObject * pyVal0 = PyList_GET_ITEM(sipPy, 0);
    PyObject * pyVal1 = PyList_GET_ITEM(sipPy, 1);
    double val0 = PyFloat_AsDouble(pyVal0);
    double val1 = PyFloat_AsDouble(pyVal1);

    QwtDoubleInterval * retVal = new QwtDoubleInterval(val0, val1);

    // Return the instance on the heap.
    *sipCppPtr = retVal;

    // Apply the normal transfer.
    return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode

    PyObject *l;
    // Create the Python list of length 2
    if ((l = PyList_New(2)) == NULL)
        return NULL;

    // Make python objects out of the numbers
    PyObject *pyVal0 = PyFloat_FromDouble( sipCpp->minValue() );
    PyObject *pyVal1 = PyFloat_FromDouble( sipCpp->maxValue() );
    PyList_SET_ITEM(l, 0, pyVal0);
    PyList_SET_ITEM(l, 1, pyVal1);

    // Return the Python list.
    return l;
    
%End

};
