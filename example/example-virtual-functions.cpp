/*
    example/example-virtual-functions.cpp -- overriding virtual functions from Python

    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include "example.h"
#include <pybind11/functional.h>

/* This is an example class that we'll want to be able to extend from Python */
class ExampleVirt  {
public:
    ExampleVirt(int state) : state(state) {
        cout << "Constructing ExampleVirt.." << endl;
    }

    ~ExampleVirt() {
        cout << "Destructing ExampleVirt.." << endl;
    }

    virtual int run(int value) {
        std::cout << "Original implementation of ExampleVirt::run(state=" << state
                  << ", value=" << value << ")" << std::endl;
        return state + value;
    }

    virtual bool run_bool() = 0;
    virtual void pure_virtual() = 0;
private:
    int state;
};

/* This is a wrapper class that must be generated */
class PyExampleVirt : public ExampleVirt {
public:
    using ExampleVirt::ExampleVirt; /* Inherit constructors */

    virtual int run(int value) {
        /* Generate wrapping code that enables native function overloading */
        PYBIND11_OVERLOAD(
            int,         /* Return type */
            ExampleVirt, /* Parent class */
            run,         /* Name of function */
            value        /* Argument(s) */
        );
    }

    virtual bool run_bool() {
        PYBIND11_OVERLOAD_PURE(
            bool,         /* Return type */
            ExampleVirt,  /* Parent class */
            run_bool,     /* Name of function */
                          /* This function has no arguments. The trailing comma
                             in the previous line is needed for some compilers */
        );
    }

    virtual void pure_virtual() {
        PYBIND11_OVERLOAD_PURE(
            void,         /* Return type */
            ExampleVirt,  /* Parent class */
            pure_virtual, /* Name of function */
                          /* This function has no arguments. The trailing comma
                             in the previous line is needed for some compilers */
        );
    }
};

int runExampleVirt(ExampleVirt *ex, int value) {
    return ex->run(value);
}

bool runExampleVirtBool(ExampleVirt* ex) {
    return ex->run_bool();
}

void runExampleVirtVirtual(ExampleVirt *ex) {
    ex->pure_virtual();
}


// Inheriting virtual methods.  We do two versions here: the repeat-everything version and the
// templated trampoline versions mentioned in docs/advanced.rst.
//
// These base classes are exactly the same, but we technically need distinct
// classes for this example code because we need to be able to bind them
// properly (pybind11, sensibly, doesn't allow us to bind the same C++ class to
// multiple python classes).
class A_Repeat {
#define A_METHODS \
public: \
    virtual int unlucky_number() = 0; \
    virtual void say_something(unsigned times) { \
        for (unsigned i = 0; i < times; i++) std::cout << "hi"; \
        std::cout << std::endl; \
    }
A_METHODS
};
class B_Repeat : public A_Repeat {
#define B_METHODS \
public: \
    int unlucky_number() override { return 13; } \
    void say_something(unsigned times) override { \
        std::cout << "B says hi " << times << " times" << std::endl; \
    } \
    virtual double lucky_number() { return 7.0; }
B_METHODS
};
class C_Repeat : public B_Repeat {
#define C_METHODS \
public: \
    int unlucky_number() override { return 4444; } \
    double lucky_number() override { return 888; }
C_METHODS
};
class D_Repeat : public C_Repeat {
#define D_METHODS // Nothing overridden.
D_METHODS
};

// Base classes for templated inheritance trampolines.  Identical to the repeat-everything version:
class A_Tpl { A_METHODS };
class B_Tpl : public A_Tpl { B_METHODS };
class C_Tpl : public B_Tpl { C_METHODS };
class D_Tpl : public C_Tpl { D_METHODS };


// Inheritance approach 1: each trampoline gets every virtual method (11 in total)
class PyA_Repeat : public A_Repeat {
public:
    using A_Repeat::A_Repeat;
    int unlucky_number() override { PYBIND11_OVERLOAD_PURE(int, A_Repeat, unlucky_number, ); }
    void say_something(unsigned times) override { PYBIND11_OVERLOAD(void, A_Repeat, say_something, times); }
};
class PyB_Repeat : public B_Repeat {
public:
    using B_Repeat::B_Repeat;
    int unlucky_number() override { PYBIND11_OVERLOAD(int, B_Repeat, unlucky_number, ); }
    void say_something(unsigned times) override { PYBIND11_OVERLOAD(void, B_Repeat, say_something, times); }
    double lucky_number() override { PYBIND11_OVERLOAD(double, B_Repeat, lucky_number, ); }
};
class PyC_Repeat : public C_Repeat {
public:
    using C_Repeat::C_Repeat;
    int unlucky_number() override { PYBIND11_OVERLOAD(int, C_Repeat, unlucky_number, ); }
    void say_something(unsigned times) override { PYBIND11_OVERLOAD(void, C_Repeat, say_something, times); }
    double lucky_number() override { PYBIND11_OVERLOAD(double, C_Repeat, lucky_number, ); }
};
class PyD_Repeat : public D_Repeat {
public:
    using D_Repeat::D_Repeat;
    int unlucky_number() override { PYBIND11_OVERLOAD(int, D_Repeat, unlucky_number, ); }
    void say_something(unsigned times) override { PYBIND11_OVERLOAD(void, D_Repeat, say_something, times); }
    double lucky_number() override { PYBIND11_OVERLOAD(double, D_Repeat, lucky_number, ); }
};

// Inheritance approach 2: templated trampoline classes.
//
// Advantages:
// - we have only 2 (template) class and 4 method declarations (one per virtual method, plus one for
//   any override of a pure virtual method), versus 4 classes and 6 methods (MI) or 4 classes and 11
//   methods (repeat).
// - Compared to MI, we also don't have to change the non-trampoline inheritance to virtual, and can
//   properly inherit constructors.
//
// Disadvantage:
// - the compiler must still generate and compile 14 different methods (more, even, than the 11
//   required for the repeat approach) instead of the 6 required for MI.  (If there was no pure
//   method (or no pure method override), the number would drop down to the same 11 as the repeat
//   approach).
template <class Base = A_Tpl>
class PyA_Tpl : public Base {
public:
    using Base::Base; // Inherit constructors
    int unlucky_number() override { PYBIND11_OVERLOAD_PURE(int, Base, unlucky_number, ); }
    void say_something(unsigned times) override { PYBIND11_OVERLOAD(void, Base, say_something, times); }
};
template <class Base = B_Tpl>
class PyB_Tpl : public PyA_Tpl<Base> {
public:
    using PyA_Tpl<Base>::PyA_Tpl; // Inherit constructors (via PyA_Tpl's inherited constructors)
    int unlucky_number() override { PYBIND11_OVERLOAD(int, Base, unlucky_number, ); }
    double lucky_number() { PYBIND11_OVERLOAD(double, Base, lucky_number, ); }
};
// Since C_Tpl and D_Tpl don't declare any new virtual methods, we don't actually need these (we can
// use PyB_Tpl<C_Tpl> and PyB_Tpl<D_Tpl> for the trampoline classes instead):
/*
template <class Base = C_Tpl> class PyC_Tpl : public PyB_Tpl<Base> {
public:
    using PyB_Tpl<Base>::PyB_Tpl;
};
template <class Base = D_Tpl> class PyD_Tpl : public PyC_Tpl<Base> {
public:
    using PyC_Tpl<Base>::PyC_Tpl;
};
*/


void initialize_inherited_virtuals(py::module &m) {
    // Method 1: repeat
    py::class_<A_Repeat, std::unique_ptr<A_Repeat>, PyA_Repeat>(m, "A_Repeat")
        .def(py::init<>())
        .def("unlucky_number", &A_Repeat::unlucky_number)
        .def("say_something", &A_Repeat::say_something);
    py::class_<B_Repeat, std::unique_ptr<B_Repeat>, PyB_Repeat>(m, "B_Repeat", py::base<A_Repeat>())
        .def(py::init<>())
        .def("lucky_number", &B_Repeat::lucky_number);
    py::class_<C_Repeat, std::unique_ptr<C_Repeat>, PyC_Repeat>(m, "C_Repeat", py::base<B_Repeat>())
        .def(py::init<>());
    py::class_<D_Repeat, std::unique_ptr<D_Repeat>, PyD_Repeat>(m, "D_Repeat", py::base<C_Repeat>())
        .def(py::init<>());

    // Method 2: Templated trampolines
    py::class_<A_Tpl, std::unique_ptr<A_Tpl>, PyA_Tpl<>>(m, "A_Tpl")
        .def(py::init<>())
        .def("unlucky_number", &A_Tpl::unlucky_number)
        .def("say_something", &A_Tpl::say_something);
    py::class_<B_Tpl, std::unique_ptr<B_Tpl>, PyB_Tpl<>>(m, "B_Tpl", py::base<A_Tpl>())
        .def(py::init<>())
        .def("lucky_number", &B_Tpl::lucky_number);
    py::class_<C_Tpl, std::unique_ptr<C_Tpl>, PyB_Tpl<C_Tpl>>(m, "C_Tpl", py::base<B_Tpl>())
        .def(py::init<>());
    py::class_<D_Tpl, std::unique_ptr<D_Tpl>, PyB_Tpl<D_Tpl>>(m, "D_Tpl", py::base<C_Tpl>())
        .def(py::init<>());

};


void init_ex_virtual_functions(py::module &m) {
    /* Important: indicate the trampoline class PyExampleVirt using the third
       argument to py::class_. The second argument with the unique pointer
       is simply the default holder type used by pybind11. */
    py::class_<ExampleVirt, std::unique_ptr<ExampleVirt>, PyExampleVirt>(m, "ExampleVirt")
        .def(py::init<int>())
        /* Reference original class in function definitions */
        .def("run", &ExampleVirt::run)
        .def("run_bool", &ExampleVirt::run_bool)
        .def("pure_virtual", &ExampleVirt::pure_virtual);

    m.def("runExampleVirt", &runExampleVirt);
    m.def("runExampleVirtBool", &runExampleVirtBool);
    m.def("runExampleVirtVirtual", &runExampleVirtVirtual);

    initialize_inherited_virtuals(m);
}
