/* test_kernel
(C) 2016 Niall Douglas http://www.nedproductions.biz/
File Created: Apr 2016


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "config.hpp"

#ifndef BOOST_KERNELTEST_TEST_KERNEL_HPP
#define BOOST_KERNELTEST_TEST_KERNEL_HPP

//#define BOOST_CATCH_CUSTOM_MAIN_DEFINED
#include "../boost-lite/include/boost/test/unit_test.hpp"
#include "../boost-lite/include/console_colours.hpp"

#define BOOST_KERNELTEST_TEST_UNIQUE_NAME2(a, b) a##b
#define BOOST_KERNELTEST_TEST_UNIQUE_NAME1(a, b) BOOST_KERNELTEST_TEST_UNIQUE_NAME2(a, b)
//! \brief A macro expanding into a unique identifier (for this compilation unit)
#define BOOST_KERNELTEST_TEST_UNIQUE_NAME(prefix) BOOST_KERNELTEST_TEST_UNIQUE_NAME1(prefix, __COUNTER__)

/*! \brief Implement a test kernel
\param category This category of test kernel. Typically 'unit' or 'integration'.
\param product Name of the product or library being tested.
\param test The name of this test kernel.
\param name The name of this test.
\param desc A pretty printable description of the test.
\param ... Code implementing the test kernel.
*/
// clang-format off
#define BOOST_KERNELTEST_TEST_KERNEL(_category, _product, _test, _name, _desc, ...)                                                                                                                                                                                                                                                 \
  \
BOOST_AUTO_TEST_CASE(_category/_product/_test/_name, _desc)                                                                                                                                                                                                                                                                         \
  {                                                                                                                                                                                                                                                                                                                            \
    \
    using namespace BOOST_KERNELTEST_V1_NAMESPACE;                                                                                                                                                                                                                                                                             \
current_test_kernel.category=#_category; \
current_test_kernel.product=#_product; \
current_test_kernel.test=#_test; \
current_test_kernel.name=#_name; \
current_test_kernel.description=_desc; \
    \
BOOST_KERNELTEST_COUT("\n\n"                                                                                                                                                                                                                                                                                                                  \
    << boost_lite::console_colours::bold << boost_lite::console_colours::blue << #_category "/" #_product "/" #_test "/" #_name ":\n"                                                                                                                                  \
    << boost_lite::console_colours::bold << boost_lite::console_colours::white << _desc << boost_lite::console_colours::normal << std::endl);                                                                                                                                                                                                                        \
    \
__VA_ARGS__;                                                                                                                                                                                                                                                                                                                   \
current_test_kernel.category=nullptr; \
current_test_kernel.product=nullptr; \
current_test_kernel.test=nullptr; \
current_test_kernel.name=nullptr; \
current_test_kernel.description=nullptr; \
  }
// clang-format on

#endif  // namespace