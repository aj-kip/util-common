/****************************************************************************

    MIT License

    Copyright (c) 2021 Aria Janke

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*****************************************************************************/

#pragma once

#include <iosfwd>
#include <stdexcept>

namespace cul {

namespace ts {

class TestAssertion;
class TestSuite;
class Unit;

/** This semantic object represents an assertion being made by a test.
 *
 *  All tests will need to return this object, which can be done by calling
 *  "test".
 *
 *  @see cul::ts::TestAssertion cul::ts::test(bool)
 */
class TestAssertion {
    friend TestAssertion test(bool);
    friend class TestSuite;
    bool value;
};

/** @returns a boolean converted into a TestAssertion, the expected return
 *           type for a unit test
 */
TestAssertion test(bool);

/** A "home grown" unit test class.
 *
 *  Tests are done a "series" at time. Each series is named and when they're
 *  finished a small summary is printed.
 *
 *  By default std::cout is used as the output stream. Which can now be
 *  changed.
 *
 *  @note limitation: cannot test for segments that are supposed to fail to
 *        compile.
 */
class TestSuite {
public:
    /** Constructs with "no series" started, but with zerod out counters. */
    TestSuite();

    /** Constructs with a series name (calling start_series) */
    explicit TestSuite(const char * series_name);
    
    TestSuite(const TestSuite &) = delete;
    TestSuite(TestSuite &&) = delete;
    
    /** calls finish_up() on destruction */
    ~TestSuite();
    
    TestSuite & operator = (const TestSuite &) = delete;
    TestSuite & operator = (TestSuite &&) = delete;
    
    /** Prints the given string and resets the success/failure counters. */
    void start_series(const char *);

    /** Performs a test, which returns an assertion.
     *
     *  This maybe called using a lambda expression that is implicitly
     *  convertible to a function pointer.
     *  @param test_func function to call for testing, must return a
     *                   TestAssertion object
     *  @see MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
     */
    template <typename Func>
    void test(Func && f) { do_test_back([&f] { return f().value; }); }

    /** Assigns a stream to write to.
     *  @warning like all "assign_*" functions, this sets an unmanaged
     *           pointer, so the stream must survive in this case *longer*
     *           than this object.
     */
    void assign_output_stream(std::ostream &);

    /** Made to mark the current position in the current source file where
     *  the test is being issued the "MACRO_CUL_TEST" is meant to automate
     *  the call to this function.
     *  @param filename name of source file
     *  @param line line number in the source file
     *  @throws if filename is nullptr or line is a negative integer
     */
    void mark_source_position(const char * filename, int line);

    /** Returns the recorded source position to "none" like this object was
     *  freshly constructed.
     */
    void unmark_source_position();

    /** "Ends" the "series" by printing a one line summary.
     *  @note this function is called automatically by the destructor
     */
    void finish_up() noexcept;

    /** @returns true if all test cases in the current are up to this point
     *           all been successful, false otherwise
     */
    bool has_successes_only() const;

    /** Causes successful test cases to not print out anything. */
    void hide_successes() { m_silence_success = true; }

    /** Causes successful test cases to print out that it passed. */
    void show_successes() { m_silence_success = false; }

private:
    template <typename Func>
    void do_test_back(Func &&);

    void print_failure(const char * exception_text = nullptr);

    void print_success();

    int m_test_count = 0;
    int m_test_successes = 0;
    bool m_silence_success = false;

    int m_source_position = 0;
    const char * m_source_file = nullptr;

    std::ostream * m_out;
};

/** Macro for marking to current position in source code for a test suite.
 *
 *  @note This macro maybe renamed to a more terse name in test source code.
 *        It is defined with a long name as not to be intrusive on the global
 *        namespace
 */
#define MACRO_MARK_POSITION_OF_CUL_TEST_SUITE(suite) \
    ([](cul::ts::TestSuite & suite) -> cul::ts::TestSuite & \
        { suite.mark_source_position(__FILE__,__LINE__); return suite; }) \
        ((suite))

/** It is sometimes desired that test share the same code that creates a
 *  context or set of commonly initialized and setup variables.
 *
 *  @param make_context
 *         this is called n + 1 number of times, where n is the number of times
 *         "start" is called by the Unit object which is passed into the
 *         function @n
 *         Lambda variable captures are disallowed (by having a function
 *         pointer passed) to encourage that contexts are isolated.
 *  @see Unit
 */
void set_context(TestSuite &, void (*make_context)(TestSuite &, Unit &));

/** Special object for "set_context", which allows unit tests to reuse code.
 *
 *  This object ensures that only one unit test is run per context.
 *
 *  @note This object may only be created for "set_context", which is the
 *        reason why it's constructor is private.
 */
class Unit {
public:
    /** Called whenever starting a unit test.
     *  @param f this function will only be called if it is the current index.
     */
    template <typename Func>
    void start(TestSuite &, Func && f);

private:
    friend class UnitAttn;
    Unit() {}
    int m_starts = 0;
    int m_index = 0;
    bool m_hit  = false;
};

// ----------------------------------------------------------------------------

class UnitAttn {
    friend void set_context(TestSuite &, void (*)(TestSuite &, Unit &));
    static Unit make_unit() { return Unit(); }
    static bool index_hit(const Unit & unit) { return unit.m_hit; }
    static void increment(Unit &);
};

// ----------------------------------------------------------------------------

template <typename Func>
void Unit::start(TestSuite & suite, Func && f) {
    if (m_index == m_starts) {
        m_hit = true;
        suite.test(std::move(f));
    }
    ++m_starts;
}

// ----------------------------------------------------------------------------

template <typename Func>
/* private */ void TestSuite::do_test_back(Func && f) {
    ++m_test_count;
    try {
        if (f()) {
            if (!m_silence_success) print_success();
            ++m_test_successes;
        } else {
            print_failure();
        }
    } catch (std::exception & exp) {
        print_failure(exp.what());
    } catch (...) {
        print_failure("<Unknown Error: unhandled exception type>");
    }

    unmark_source_position();
}

} // end of ts namespace -> into ::cul

} // end of cul namespace
