#include <algorithm>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>
#include <ranges>
#include <concepts>

using namespace std;

// // Range concept
// template <BoundedRange R>
// requires Sortable<R>
// void sort(R &r) {
//   return sort(begin(r),end(r));
// }

template <typename C>
using Value_type = typename C::value_type; // the type of C's elements.

template <typename C>
using Iterator_type = typename C::iterator; // C's iterator type.

template <typename S>
concept Sequence = requires(S a) {
  typename Value_type<S>;                     // S must hava a value type
  typename Iterator_type<S>;                  // S must hava an iterator type

  { begin(a) } -> same_as<Iterator_type<S>>;           // begin(a) must return an iterator
  { end(a) } -> same_as<Iterator_type<S>>;             // end(a) must return an iterator

  requires same_as<Value_type<S>, Value_type<Iterator_type<S>>>;
  requires input_iterator<Iterator_type<S>>;
  requires integral<Value_type<S>>;           // only integral types allowed.
};

template <Sequence Seq, typename Num> Num sum(Seq s, Num v) {
  for (const auto& x : s)
  {
    v+=x;
  }
  return v;
}

// Print phone list.
template<template<class> class C,class T>
ostream &operator<<(ostream &os, const C<T> &c) {
  os << "{ ";
  for (const auto& x : c)
  {
    os << x << " ";
  }
  os << "}";

  return os;
}

template <typename N>
concept Number = requires(N n) {
  requires integral<N>;
};

template <typename R>
concept Range = requires(R r) {
  typename Value_type<R>;                     // S must hava a value type
  typename Iterator_type<R>;                  // S must hava an iterator type

  { begin(r) } -> same_as<Iterator_type<R>>;           // begin(a) must return an iterator
  { end(r) } -> same_as<Iterator_type<R>>;             // end(a) must return an iterator

  requires same_as<Value_type<R>, Value_type<Iterator_type<R>>>;
};

template <Range R, Number Val> // Range concept if defined up.
Val accumulate(const R &r, Val res = 0) {
  for (auto p= begin(r); p!=end(r); ++p)
    res += *p;
  return res;
}

template <Range R, Number Val, typename F> // Range concept if defined up.
Val accumulate(const R &r, Val res, F fn) {
  for (auto p= begin(r); p!=end(r); ++p)
    res = fn(res,*p);
  return res;
}

////////////////////////////////////////////////////////////////////////////////
//
// MAIN FUNCTION
//
////////////////////////////////////////////////////////////////////////////////
int main()
{
  vector<int> v{22,14,6,18,10};
  // list<double> l{0.5,0.7,1.4,1.7,1.8,2.3};
  // set<string> s{"Gustavo"," is programming"," hard ","in ","c++20"};

  ranges::sort(v); // sort using ranges in c++20.
  cout << "Sorted vector = " << v << endl;

  cout << "Accumulate of v is " << accumulate(v,0) << endl;

  cout << "Multiplicative of v is " << accumulate(v,1,[](int res, int op){return res*op;}) << endl;

  string line = "Gustavo is programming hard in c++20"s;
  auto letter = line[10];

  string ini="";

  int number = 0;
  double num_float = 0.0;

  cout << "Sum of vector is " << sum(v,number) << endl;
  // cout << "Sum of list is " << sum(l,num_float) << endl;
  // cout << "Sum of set is " << sum(s,ini) << endl;
  cout << "Sum of string is " << sum(line,letter) << endl;

  return 0;
}
