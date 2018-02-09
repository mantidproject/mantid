
from __future__ import (absolute_import, division, print_function)
# translation of FFT.for
import math


def fortran_FFT(kk, a, sn):
    # INU=2**(n-1) in Fortran terms
    n = 2 * kk  # n=orig, length of data
    n2 = n / 2  # n2 orig
    e = sn * math.pi
    j = n2  # now reduced by 1
    # loop DO 1
    for i in range(1, n):  # i less by 1
        k = kk - 1  # orig, log2(something)
        m = n2  # orig
        if(i - j) < 0:  # both less 1
            print ("swapping", i, " with ", j)
            b = a[j]  # label 2
            a[j] = a[i]
            a[i] = b
        while(True):  # label 4
            L = j - m  # L reduced by 1
            if(L >= 0):
                # label 3
                j = L  # reduced by 1
                m = 2**(k - 1)  # orig
                k = k - 1  # orig, log2
                if(k <= 0):
                    # goto label 5
                    break
                # else continue loop label 4
            else:
                break
        j = j + m  # label 5. orig m, reduced j
    # 1 continue
    k = 1
    while(k < n):
        L = k + k
        c2 = math.cos(e)
        eq = complex(c2, math.sin(e))
        el = 1.
        c2 = c2 + c2
        for m in range(k):  # DO 10..
            for i in range(m, n, L):  # DO 7
                j = i + k
                b = el * a[j]
                a[j] = a[i] - b
                a[i] = a[i] + b
            en = eq * c2 - el
            x = en.real
            y = en.imag
            en = en * 0.5 * (3. - x * x - y * y)
            el = eq
            eq = en
        k = L
        e = 0.5 * e
    return a
