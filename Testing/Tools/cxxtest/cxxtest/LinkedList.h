// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/Flags.h>

namespace CxxTest
{
    struct List;
    class Link;

    struct List
    {
        Link *_head;
        Link *_tail;

        void initialize();

        Link *head();
        const Link *head() const;
        Link *tail();
        const Link *tail() const;

        bool empty() const;
        unsigned size() const;
        Link *nth( unsigned n );

        void activateAll();
        void leaveOnly( const Link &link );
    };

    class Link
    {
    public:
        Link();
        virtual ~Link();

        bool active() const;
        void setActive( bool value = true );

        Link *justNext();
        Link *justPrev();

        Link *next();
        Link *prev();
        const Link *next() const;
        const Link *prev() const;

        void attach( List &l );
        void detach( List &l );

    private:
        Link *_next;
        Link *_prev;
        bool _active;

        Link( const Link & );
        Link &operator=( const Link & );
    };
}
