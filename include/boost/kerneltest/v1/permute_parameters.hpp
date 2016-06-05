/* permute_parameters
(C) 2016 Niall Douglas http://www.nedprod.com/
File Created: Apr 2016
*/

#include "config.hpp"

#ifndef BOOST_KERNELTEST_PERMUTE_PARAMETERS_HPP
#define BOOST_KERNELTEST_PERMUTE_PARAMETERS_HPP

#include "../boost-lite/include/type_traits.hpp"

#include <array>
#include <vector>

template <class T> struct print_type
{
private:
  print_type() {}
};

namespace boost_lite
{
  namespace type_traits
  {
#if 0
    namespace detail
    {
      template <size_t N> struct Char
      {
        char foo[N];
      };
      template <class T> constexpr inline auto has_constexpr_size(T &&v) { return Char<(v.size(), 2)>(); }
      template <class T> constexpr inline Char<1> has_constexpr_size(...) { return Char<1>(); }
    }
    //! Returns true if the instance of v has a constexpr size()
    template <class T> constexpr inline bool has_constexpr_size(T &&v) { return 2 == sizeof(detail::has_constexpr_size<T>(std::forward<T>(v))); }

    // static_assert(std::array<int, 2>().size() == 2, "foo");
    static_assert(has_constexpr_size(std::array<int, 2>()), "foo");
#else
    //! Returns true if the instance of v has a constexpr size()
    template <class T> struct has_constexpr_size : std::false_type
    {
    };
    template <class T, size_t N> struct has_constexpr_size<std::array<T, N>> : std::true_type
    {
    };
#endif
  }
}

BOOST_KERNELTEST_V1_NAMESPACE_BEGIN

namespace detail
{
  template <class ParamSequence, bool = boost_lite::type_traits::has_constexpr_size<ParamSequence>::value> struct permutation_results_type
  {
    template <class T> using type = std::vector<T>;
    constexpr ParamSequence operator()(size_t no) const { return ParamSequence(no); }
  };
  template <class ParamSequence> struct permutation_results_type<ParamSequence, true>
  {
    template <class T> using type = std::array<T, ParamSequence().size()>;
    constexpr ParamSequence operator()(size_t no) const { return ParamSequence(); }
  };
  template <class T> constexpr T make_permutation_results_type(size_t no) { return permutation_results_type<T>()(no); }

  template <class ParamSequence, class Callable> struct result_of_parameter_permute;
  template <class OutcomeType, class... Types, class... Excess, class Callable> struct result_of_parameter_permute<parameters<OutcomeType, parameters<Types...>, Excess...>, Callable>
  {
    using type = decltype(std::declval<Callable>()(std::declval<Types>()...));
  };
  template <class U, class... Types, size_t... Idxs> auto call_f_with_parameters(U &&f, const parameters<Types...> &params, std::index_sequence<Idxs...>) { return f(std::get<Idxs>(params)...); }

  template <class T> bool check_result(const outcome<T> &kernel_outcome, const outcome<T> &shouldbe) { return kernel_outcome == shouldbe; };
  template <class T> bool check_result(const result<T> &kernel_outcome, const result<T> &shouldbe) { return kernel_outcome == shouldbe; };
  template <class T> bool check_result(const option<T> &kernel_outcome, const option<T> &shouldbe) { return kernel_outcome == shouldbe; };

  // If should be has type void, we only care kernel_outcome has a value
  template <class T> bool check_result(const outcome<T> &kernel_outcome, const outcome<void> &shouldbe)
  {
    if(kernel_outcome.has_value() && shouldbe.has_value())
      return kernel_outcome.has_value() == shouldbe.has_value();
    else
      return kernel_outcome == shouldbe;
  };
  template <class T> bool check_result(const result<T> &kernel_outcome, const result<void> &shouldbe)
  {
    if(kernel_outcome.has_value() && shouldbe.has_value())
      return kernel_outcome.has_value() == shouldbe.has_value();
    else
      return kernel_outcome == shouldbe;
  };
  template <class T> bool check_result(const option<T> &kernel_outcome, const option<void> &shouldbe)
  {
    if(kernel_outcome.has_value() && shouldbe.has_value())
      return kernel_outcome.has_value() == shouldbe.has_value();
    else
      return kernel_outcome == shouldbe;
  };
}

/*! \brief A parameter permuter instance
\tparam is_mt True if this is a multithreaded parameter permuter
\tparam ParamSequence A sequence of parameter calls
*/
template <bool is_mt, class ParamSequence, class... Hooks> class parameter_permuter
{
  ParamSequence _params;
  std::tuple<Hooks...> _hooks;

  // syntax helper for MSVC :)
  using _permutation_results_type = typename detail::permutation_results_type<ParamSequence>;

public:
  //! True if this parameter permuter is multithreaded
  static constexpr bool is_multithreaded = is_mt;
  //! The type of the sequence of parameters
  using parameter_sequence_type = ParamSequence;
  //! The type of an individual parameter set
  using parameter_sequence_value_type = typename parameter_sequence_type::value_type;
  //! The type of the outcome from an individual parameter set
  using outcome_type = typename parameters_element<0, parameter_sequence_value_type>::type;
  //! Accessor for the outcome from an individual parameter set
  static constexpr const outcome_type &outcome_value(const parameter_sequence_value_type &v) { return std::get<0>(v); }
  //! The number of parameters in an individual parameter set
  static constexpr size_t parameters_size = BOOST_KERNELTEST_V1_NAMESPACE::parameters_size<parameter_sequence_value_type>::value - 1;
  //! The type of the parameter at index N
  template <size_t N> using parameter_type = typename parameters_element<1 + N, parameter_sequence_value_type>::type;
  //! Accessor for the parameter at index N
  template <size_t N> static constexpr const parameter_type<N> &parameter_value(const parameter_sequence_value_type &v) { return std::get<N + 1>(v); }
  //! The type of the results returned by the call operator. Can be array<> or vector<> depending on ParamSequence
  template <class T> using permutation_results_type = typename _permutation_results_type::template type<T>;

  //! Constructs an instance. Best to use mt_permute_parameters() or st_permute_parameters() instead.
  constexpr parameter_permuter(ParamSequence &&params, std::tuple<Hooks...> &&hooks)
      : _params(std::move(params))
      , _hooks(std::move(hooks))
  {
  }

  //! Returns the parameter sequence this permuter was constructed with
  const ParamSequence &parameter_sequence() const { return _params; }

  /*! Permute the callable f with this parameter permuter, returning a vector of results.
  \return An array or vector of results (depends on ParamSequence).
  \throws bad_alloc Failure to allocate the vector of results.
  \throws anything Any exception thrown by any call of the callable f
  \param f Some callable with callspec result(typename ParamSequence::value_type ...)
  */
  template <class U> auto operator()(U &&f) const
  {
    using return_type = typename detail::result_of_parameter_permute<parameter_sequence_value_type, U>::type;
    using return_type_as_if_void = typename return_type::template rebind<void>;
    static_assert(outcome_type::has_value_type ? (std::is_constructible<outcome_type, return_type>::value) : (std::is_constructible<outcome_type, return_type_as_if_void>::value), "Return type of callable is not compatible with the parameter outcome type");
    permutation_results_type<return_type> results(detail::make_permutation_results_type<permutation_results_type<return_type>>(_params.size()));
    permutation_results_type<const parameter_sequence_value_type *> params(detail::make_permutation_results_type<permutation_results_type<const parameter_sequence_value_type *>>(_params.size()));
    {
      auto it(params.begin());
      for(auto &i : _params)
        *it++ = &i;
    }
    auto call_f = [&](size_t idx) {
      using callable_parameters_type = parameter_type<0>;
      const callable_parameters_type &p = parameter_value<0>(*params[idx]);
      // TODO: Hooks
      results[idx] = detail::call_f_with_parameters(std::forward<U>(f), p, std::make_index_sequence<BOOST_KERNELTEST_V1_NAMESPACE::parameters_size<callable_parameters_type>::value>());
    };
#ifdef _OPENMP
    if(is_multithreaded)
    {
#pragma omp parallel for
      for(size_t n = 0; n < results.size(); n++)
        call_f(n);
    }
    else
#endif
    {
      for(size_t n = 0; n < results.size(); n++)
        call_f(n);
    }
    return results;
  }

  /*! Checks a sequence of results against what they ought to be, calling the callable f with the results
  \return True if all the results match
  \throws invalid_argument If the results passed is not of the same length as the parameter permute sequence
  \param results A sequence of results to check
  \param fail Some callable with callspec bool(size_t, value, shouldbe) called if the values do not match
  \param pass Some callable with callspec bool(size_t, value, shouldbe) called if the values match
  */
  template <class U, class V, class W, typename std::enable_if<boost_lite::type_traits::is_sequence<U>::value, bool>::type = true> bool check(U &&sequence, V &&fail, W &&pass) const
  {
    if(sequence.size() != _params.size())
      throw std::invalid_argument("sequence to check does not have same length as parameter permute sequence");
    bool ret = true;
    auto it(sequence.cbegin());
    size_t idx = 0;
    for(auto &i : _params)
    {
      const outcome_type &shouldbe = outcome_value(i);
      const auto &outcome = *it;
      if(detail::check_result(outcome, shouldbe))
      {
        if(!pass(idx, outcome, shouldbe))
          ret = false;
      }
      else
      {
        if(!fail(idx, outcome, shouldbe))
          ret = false;
      }
      ++it;
      ++idx;
    }
    return ret;
  }
  //! \overload
  template <class U, class V> bool check(U &&sequence, V &&fail) const
  {
    return check(std::forward<U>(sequence), std::forward<V>(fail), [](size_t, const auto &, const auto &) { return true; });
  }
};

namespace detail
{
  template <class ParamSequence, class OutcomeType, class... Parameters> struct is_parameters_sequence_type_valid : std::false_type
  {
  };
  template <class OutcomeType, class... Parameters, template <class...> class Container> struct is_parameters_sequence_type_valid<Container<parameters<OutcomeType, Parameters...>>, OutcomeType, Parameters...> : std::true_type
  {
  };
}

/*! \brief Create a multithreaded parameter permuter
\tparam OutcomeType An outcome<T>, result<T> or option<T> for the outcome of the test kernel
\tparam InputTypes The types of the parameters of the test kernel
\tparam Sequence The type of the sequence containing the parameter sets
\tparam Hooks The types of any pretest or posttest hooks
\param seq The sequence of parameter sets
*/
template <class OutcomeType, class... Parameters, class Sequence, class... Hooks, typename = typename std::enable_if<detail::is_parameters_sequence_type_valid<Sequence, OutcomeType, Parameters...>::value>::type> constexpr parameter_permuter<true, Sequence> mt_permute_parameters(Sequence &&seq, Hooks &&... hooks)
{
  return parameter_permuter<true, Sequence, Hooks...>(std::forward<Sequence>(seq), std::tuple<Hooks...>(std::forward<Hooks>(hooks)...));
}
//! \overload
template <class... Parameters, class... Hooks> constexpr auto mt_permute_parameters(std::initializer_list<parameters<Parameters...>> seq, Hooks &&... hooks)
{
  return parameter_permuter<true, std::initializer_list<parameters<Parameters...>>, Hooks...>(std::move(seq), std::tuple<Hooks...>(std::forward<Hooks>(hooks)...));
}

namespace detail
{
  template <class Sequence, class U> class pretty_print_failure_impl
  {
    const Sequence &_sequence;
    U _f;

    class _print_params
    {
      template <bool first> static void _do() {}
      template <bool first, class T, class... Types> static void _do(T &&v, Types &&... vs)
      {
        if(!first)
          std::cout << ", ";
        std::cout << v;
        _do<false>(std::forward<Types>(vs)...);
      };

    public:
      template <class... Types> void operator()(Types &&... vs) const { _do<true>(std::forward<Types>(vs)...); }
    };

  public:
    template <class V>
    pretty_print_failure_impl(const Sequence &sequence, V &&f)
        : _sequence(sequence)
        , _f(std::forward<V>(f))
    {
    }
    template <class T, class U> bool operator()(size_t idx, const T &result, const U &shouldbe) const
    {
      using namespace boost_lite::console_colours;
      std::cout << "  " << yellow << (idx + 1) << "/" << _sequence.size() << ": " << normal << "kernel(";
      auto parameter_sequence_item_it = _sequence.begin();
      std::advance(parameter_sequence_item_it, idx);
      const auto &pars = std::get<1>(*parameter_sequence_item_it);
      using pars_type = typename std::decay<decltype(pars)>::type;
      detail::call_f_with_parameters(_print_params(), pars, std::make_index_sequence<parameters_size<pars_type>::value>());
      std::cout << ")\n";
      std::cout << "    " << bold << red << "FAILED" << normal << " (should be " << bold << shouldbe << normal << ", was " << bold << result << normal << ")" << std::endl;
      _f(result, shouldbe);
      return false;
    }
  };
  template <class Sequence, class U> class pretty_print_success_impl
  {
    const Sequence &_sequence;
    U _f;

  public:
    template <class V>
    pretty_print_success_impl(const Sequence &sequence, V &&f)
        : _sequence(sequence)
        , _f(std::forward<V>(f))
    {
    }
    template <class T, class U> bool operator()(size_t idx, const T &result, const U &) const
    {
      using namespace boost_lite::console_colours;
      std::cout << bold << green << result << normal << std::endl;
      _f(result, shouldbe);
      return true;
    }
  };
}
//! Colourfully prints a failed result
template <class Sequence, class U> detail::pretty_print_failure_impl<Sequence, U> pretty_print_failure(const Sequence &s, U &&f)
{
  return detail::pretty_print_failure_impl<Sequence, U>(s, std::forward<U>(f));
}
//! \overload
template <class Sequence> auto pretty_print_failure(const Sequence &s)
{
  return pretty_print_failure(s, [](const auto &, const auto &) {});
}
//! Colourfully prints a successful result
template <class Sequence, class U> detail::pretty_print_success_impl<Sequence, U> pretty_print_success(const Sequence &s, U &&f)
{
  return detail::pretty_print_success_impl<Sequence, U>(s, std::forward<U>(f));
}
//! \overload
template <class Sequence> auto pretty_print_success(const Sequence &s)
{
  return pretty_print_success(s, [](const auto &, const auto &) {});
}

BOOST_KERNELTEST_V1_NAMESPACE_END

#endif  // namespace
