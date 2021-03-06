/*
 * Copyright © 2018  Google, Inc.
 * Copyright © 2019  Facebook, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 * Facebook Author(s): Behdad Esfahbod
 */

#ifndef HB_ITER_HH
#define HB_ITER_HH

#include "hb.hh"
#include "hb-algs.hh"
#include "hb-meta.hh"


/* Unified iterator object.
 *
 * The goal of this template is to make the same iterator interface
 * available to all types, and make it very easy and compact to use.
 * hb_iter_tator objects are small, light-weight, objects that can be
 * copied by value.  If the collection / object being iterated on
 * is writable, then the iterator returns lvalues, otherwise it
 * returns rvalues.
 *
 * TODO Document more.
 *
 * If iterator implementation implements operator!=, then can be
 * used in range-based for loop.  That comes free if the iterator
 * is random-access.  Otherwise, the range-based for loop incurs
 * one traversal to find end(), which can be avoided if written
 * as a while-style for loop, or if iterator implements a faster
 * __end__() method.
 * TODO When opting in for C++17, address this by changing return
 * type of .end()?
 */


/*
 * Base classes for iterators.
 */

/* Base class for all iterators. */
template <typename iter_t, typename Item = typename iter_t::__item_t__>
struct hb_iter_t
{
  typedef Item item_t;
  static constexpr unsigned item_size = hb_static_size (Item);
  static constexpr bool is_iterator = true;
  static constexpr bool is_random_access_iterator = false;
  static constexpr bool is_sorted_iterator = false;

  private:
  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
  const iter_t* thiz () const { return static_cast<const iter_t *> (this); }
        iter_t* thiz ()       { return static_cast<      iter_t *> (this); }
  public:

  /* TODO:
   * Port operators below to use hb_enable_if to sniff which method implements
   * an operator and use it, and remove hb_iter_fallback_mixin_t completely. */

  /* Operators. */
  iter_t iter () const { return *thiz(); }
  iter_t operator + () const { return *thiz(); }
  iter_t begin () const { return *thiz(); }
  iter_t end () const { return thiz()->__end__ (); }
  explicit operator bool () const { return thiz()->__more__ (); }
  unsigned len () const { return thiz()->__len__ (); }
  /* The following can only be enabled if item_t is reference type.  Otherwise
   * it will be returning pointer to temporary rvalue.
   * TODO Use a wrapper return type to fix for non-reference type. */
  template <typename T = item_t,
	    hb_enable_if (hb_is_reference (T))>
  hb_remove_reference<item_t>* operator -> () const { return hb_addressof (**thiz()); }
  item_t operator * () const { return thiz()->__item__ (); }
  item_t operator * () { return thiz()->__item__ (); }
  item_t operator [] (unsigned i) const { return thiz()->__item_at__ (i); }
  item_t operator [] (unsigned i) { return thiz()->__item_at__ (i); }
  iter_t& operator += (unsigned count) &  { thiz()->__forward__ (count); return *thiz(); }
  iter_t  operator += (unsigned count) && { thiz()->__forward__ (count); return *thiz(); }
  iter_t& operator ++ () &  { thiz()->__next__ (); return *thiz(); }
  iter_t  operator ++ () && { thiz()->__next__ (); return *thiz(); }
  iter_t& operator -= (unsigned count) &  { thiz()->__rewind__ (count); return *thiz(); }
  iter_t  operator -= (unsigned count) && { thiz()->__rewind__ (count); return *thiz(); }
  iter_t& operator -- () &  { thiz()->__prev__ (); return *thiz(); }
  iter_t  operator -- () && { thiz()->__prev__ (); return *thiz(); }
  iter_t operator + (unsigned count) const { auto c = thiz()->iter (); c += count; return c; }
  friend iter_t operator + (unsigned count, const iter_t &it) { return it + count; }
  iter_t operator ++ (int) { iter_t c (*thiz()); ++*thiz(); return c; }
  iter_t operator - (unsigned count) const { auto c = thiz()->iter (); c -= count; return c; }
  iter_t operator -- (int) { iter_t c (*thiz()); --*thiz(); return c; }
  template <typename T>
  iter_t& operator >> (T &v) &  { v = **thiz(); ++*thiz(); return *thiz(); }
  template <typename T>
  iter_t  operator >> (T &v) && { v = **thiz(); ++*thiz(); return *thiz(); }
  template <typename T>
  iter_t& operator << (const T v) &  { **thiz() = v; ++*thiz(); return *thiz(); }
  template <typename T>
  iter_t  operator << (const T v) && { **thiz() = v; ++*thiz(); return *thiz(); }

  protected:
  hb_iter_t () {}
  hb_iter_t (const hb_iter_t &o HB_UNUSED) {}
  void operator = (const hb_iter_t &o HB_UNUSED) {}
};

#define HB_ITER_USING(Name) \
  using item_t = typename Name::item_t; \
  using Name::begin; \
  using Name::end; \
  using Name::item_size; \
  using Name::is_iterator; \
  using Name::iter; \
  using Name::operator bool; \
  using Name::len; \
  using Name::operator ->; \
  using Name::operator *; \
  using Name::operator []; \
  using Name::operator +=; \
  using Name::operator ++; \
  using Name::operator -=; \
  using Name::operator --; \
  using Name::operator +; \
  using Name::operator -; \
  using Name::operator >>; \
  using Name::operator <<; \
  static_assert (true, "")

/* Returns iterator / item type of a type. */
template <typename Iterable>
using hb_iter_type = decltype (hb_deref (hb_declval (Iterable)).iter ());
template <typename Iterable>
using hb_item_type = decltype (*hb_deref (hb_declval (Iterable)).iter ());


template <typename> struct hb_array_t;

struct
{
  template <typename T> hb_iter_type<T>
  operator () (T&& c) const
  { return hb_deref (hb_forward<T> (c)).iter (); }

  /* Specialization for C arrays. */

  template <typename Type> inline hb_array_t<Type>
  operator () (Type *array, unsigned int length) const
  { return hb_array_t<Type> (array, length); }

  template <typename Type, unsigned int length> hb_array_t<Type>
  operator () (Type (&array)[length]) const
  { return hb_array_t<Type> (array, length); }

}
HB_FUNCOBJ (hb_iter);

/* Mixin to fill in what the subclass doesn't provide. */
template <typename iter_t, typename item_t = typename iter_t::__item_t__>
struct hb_iter_fallback_mixin_t
{
  private:
  /* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
  const iter_t* thiz () const { return static_cast<const iter_t *> (this); }
        iter_t* thiz ()       { return static_cast<      iter_t *> (this); }
  public:

  /* Access: Implement __item__(), or __item_at__() if random-access. */
  item_t __item__ () const { return (*thiz())[0]; }
  item_t __item_at__ (unsigned i) const { return *(*thiz() + i); }

  /* Termination: Implement __more__(), or __len__() if random-access. */
  bool __more__ () const { return thiz()->len (); }
  unsigned __len__ () const
  { iter_t c (*thiz()); unsigned l = 0; while (c) { c++; l++; }; return l; }

  /* Advancing: Implement __next__(), or __forward__() if random-access. */
  void __next__ () { *thiz() += 1; }
  void __forward__ (unsigned n) { while (n--) ++*thiz(); }

  /* Rewinding: Implement __prev__() or __rewind__() if bidirectional. */
  void __prev__ () { *thiz() -= 1; }
  void __rewind__ (unsigned n) { while (n--) --*thiz(); }

  /* Range-based for: Implement __end__() if can be done faster,
   * and operator!=. */
  iter_t __end__ () const
  {
    if (thiz()->is_random_access_iterator)
      return *thiz() + thiz()->len ();
    /* Above expression loops twice. Following loops once. */
    auto it = *thiz();
    while (it) ++it;
    return it;
  }

  protected:
  hb_iter_fallback_mixin_t () {}
  hb_iter_fallback_mixin_t (const hb_iter_fallback_mixin_t &o HB_UNUSED) {}
  void operator = (const hb_iter_fallback_mixin_t &o HB_UNUSED) {}
};

template <typename iter_t, typename item_t = typename iter_t::__item_t__>
struct hb_iter_with_fallback_t :
  hb_iter_t<iter_t, item_t>,
  hb_iter_fallback_mixin_t<iter_t, item_t>
{
  protected:
  hb_iter_with_fallback_t () {}
  hb_iter_with_fallback_t (const hb_iter_with_fallback_t &o HB_UNUSED) :
    hb_iter_t<iter_t, item_t> (o),
    hb_iter_fallback_mixin_t<iter_t, item_t> (o) {}
  void operator = (const hb_iter_with_fallback_t &o HB_UNUSED) {}
};

/*
 * Meta-programming predicates.
 */

/* hb_is_iterable() */

template <typename T>
struct hb_is_iterable
{
  private:

  template <typename U>
  static auto impl (hb_priority<1>) -> decltype (hb_declval (U).iter (), hb_true_t ());

  template <typename>
  static hb_false_t impl (hb_priority<0>);

  public:

  enum { value = decltype (impl<T> (hb_prioritize))::value };
};
#define hb_is_iterable(Iterable) hb_is_iterable<Iterable>::value

/* TODO Add hb_is_iterable_of().
 * TODO Add random_access / sorted variants. */

/* hb_is_iterator() / hb_is_random_access_iterator() / hb_is_sorted_iterator() */

template <typename Iter, typename Item>
static inline char _hb_is_iterator_of (hb_priority<0>, const void *) { return 0; }
template <typename Iter,
	  typename Item,
	  typename Item2 = typename Iter::item_t,
	  hb_enable_if (hb_is_cr_convertible_to (Item2, Item))>
static inline int _hb_is_iterator_of (hb_priority<2>, hb_iter_t<Iter, Item2> *) { return 0; }

template<typename Iter, typename Item>
struct hb_is_iterator_of { enum {
  value = sizeof (int) == sizeof (_hb_is_iterator_of<Iter, Item> (hb_prioritize, hb_declval (Iter*))) }; };
#define hb_is_iterator_of(Iter, Item) hb_is_iterator_of<Iter, Item>::value
#define hb_is_iterator(Iter) hb_is_iterator_of (Iter, typename Iter::item_t)

#define hb_is_random_access_iterator_of(Iter, Item) \
  hb_is_iterator_of (Iter, Item) && Iter::is_random_access_iterator
#define hb_is_random_access_iterator(Iter) \
  hb_is_random_access_iterator_of (Iter, typename Iter::item_t)

#define hb_is_sorted_iterator_of(Iter, Item) \
  hb_is_iterator_of (Iter, Item) && Iter::is_sorted_iterator
#define hb_is_sorted_iterator(Iter) \
  hb_is_sorted_iterator_of (Iter, typename Iter::item_t)


/* Range-based 'for' for iterables. */

template <typename Iterable,
	  hb_requires (hb_is_iterable (Iterable))>
static inline auto begin (Iterable&& iterable) HB_AUTO_RETURN (hb_iter (iterable).begin ())

template <typename Iterable,
	  hb_requires (hb_is_iterable (Iterable))>
static inline auto end (Iterable&& iterable) HB_AUTO_RETURN (hb_iter (iterable).end ())

/* begin()/end() are NOT looked up non-ADL.  So each namespace must declare them.
 * Do it for namespace OT. */
namespace OT {

template <typename Iterable,
	  hb_requires (hb_is_iterable (Iterable))>
static inline auto begin (Iterable&& iterable) HB_AUTO_RETURN (hb_iter (iterable).begin ())

template <typename Iterable,
	  hb_requires (hb_is_iterable (Iterable))>
static inline auto end (Iterable&& iterable) HB_AUTO_RETURN (hb_iter (iterable).end ())

}


/*
 * Adaptors, combiners, etc.
 */

template <typename Lhs, typename Rhs,
	  hb_requires (hb_is_iterator (Lhs))>
static inline auto
operator | (Lhs&& lhs, Rhs&& rhs) HB_AUTO_RETURN (hb_forward<Rhs> (rhs) (hb_forward<Lhs> (lhs)))

/* hb_map(), hb_filter(), hb_reduce() */

template <typename Iter, typename Proj,
	 hb_requires (hb_is_iterator (Iter))>
struct hb_map_iter_t :
  hb_iter_t<hb_map_iter_t<Iter, Proj>,
	    decltype (hb_get (hb_declval (Proj), *hb_declval (Iter)))>
{
  hb_map_iter_t (const Iter& it, Proj f_) : it (it), f (f_) {}

  typedef decltype (hb_get (hb_declval (Proj), *hb_declval (Iter))) __item_t__;
  static constexpr bool is_random_access_iterator = Iter::is_random_access_iterator;
  __item_t__ __item__ () const { return hb_get (f.get (), *it); }
  __item_t__ __item_at__ (unsigned i) const { return hb_get (f.get (), it[i]); }
  bool __more__ () const { return bool (it); }
  unsigned __len__ () const { return it.len (); }
  void __next__ () { ++it; }
  void __forward__ (unsigned n) { it += n; }
  void __prev__ () { --it; }
  void __rewind__ (unsigned n) { it -= n; }
  hb_map_iter_t __end__ () const { return hb_map_iter_t (it.end (), f); }
  bool operator != (const hb_map_iter_t& o) const
  { return it != o.it || f != o.f; }

  private:
  Iter it;
  hb_reference_wrapper<Proj> f;
};

template <typename Proj>
struct hb_map_iter_factory_t
{
  hb_map_iter_factory_t (Proj f) : f (f) {}

  template <typename Iter,
	    hb_requires (hb_is_iterator (Iter))>
  hb_map_iter_t<Iter, Proj>
  operator () (Iter it)
  { return hb_map_iter_t<Iter, Proj> (it, f); }

  private:
  Proj f;
};
struct
{
  template <typename Proj>
  hb_map_iter_factory_t<Proj>
  operator () (Proj&& f) const
  { return hb_map_iter_factory_t<Proj> (f); }
}
HB_FUNCOBJ (hb_map);

template <typename Iter, typename Pred, typename Proj,
	 hb_requires (hb_is_iterator (Iter))>
struct hb_filter_iter_t :
  hb_iter_with_fallback_t<hb_filter_iter_t<Iter, Pred, Proj>,
			  typename Iter::item_t>
{
  hb_filter_iter_t (const Iter& it_, Pred p_, Proj f_) : it (it_), p (p_), f (f_)
  { while (it && !hb_has (p.get (), hb_get (f.get (), *it))) ++it; }

  typedef typename Iter::item_t __item_t__;
  static constexpr bool is_sorted_iterator = Iter::is_sorted_iterator;
  __item_t__ __item__ () const { return *it; }
  bool __more__ () const { return bool (it); }
  void __next__ () { do ++it; while (it && !hb_has (p.get (), hb_get (f.get (), *it))); }
  void __prev__ () { --it; }
  hb_filter_iter_t __end__ () const { return hb_filter_iter_t (it.end (), p, f); }
  bool operator != (const hb_filter_iter_t& o) const
  { return it != o.it || p != o.p || f != o.f; }

  private:
  Iter it;
  hb_reference_wrapper<Pred> p;
  hb_reference_wrapper<Proj> f;
};
template <typename Pred, typename Proj>
struct hb_filter_iter_factory_t
{
  hb_filter_iter_factory_t (Pred p, Proj f) : p (p), f (f) {}

  template <typename Iter,
	    hb_requires (hb_is_iterator (Iter))>
  hb_filter_iter_t<Iter, Pred, Proj>
  operator () (Iter it)
  { return hb_filter_iter_t<Iter, Pred, Proj> (it, p, f); }

  private:
  Pred p;
  Proj f;
};
struct
{
  template <typename Pred = decltype ((hb_identity)),
	    typename Proj = decltype ((hb_identity))>
  hb_filter_iter_factory_t<Pred, Proj>
  operator () (Pred&& p = hb_identity, Proj&& f = hb_identity) const
  { return hb_filter_iter_factory_t<Pred, Proj> (p, f); }
}
HB_FUNCOBJ (hb_filter);

template <typename Redu, typename InitT>
struct hb_reduce_t
{
  hb_reduce_t (Redu r, InitT init_value) : r (r), init_value (init_value) {}

  template <typename Iter,
	    hb_requires (hb_is_iterator (Iter)),
	    typename AccuT = decltype (hb_declval (Redu) (hb_declval (InitT), hb_declval (typename Iter::item_t)))>
  AccuT
  operator () (Iter it)
  {
    AccuT value = init_value;
    for (; it; ++it)
      value = r (value, *it);
    return value;
  }

  private:
  Redu r;
  InitT init_value;
};
struct
{
  template <typename Redu, typename InitT>
  hb_reduce_t<Redu, InitT>
  operator () (Redu&& r, InitT init_value) const
  { return hb_reduce_t<Redu, InitT> (r, init_value); }
}
HB_FUNCOBJ (hb_reduce);


/* hb_zip() */

template <typename A, typename B>
struct hb_zip_iter_t :
  hb_iter_t<hb_zip_iter_t<A, B>,
	    hb_pair_t<typename A::item_t, typename B::item_t>>
{
  hb_zip_iter_t () {}
  hb_zip_iter_t (const A& a, const B& b) : a (a), b (b) {}

  typedef hb_pair_t<typename A::item_t, typename B::item_t> __item_t__;
  static constexpr bool is_random_access_iterator =
    A::is_random_access_iterator &&
    B::is_random_access_iterator;
  static constexpr bool is_sorted_iterator =
    A::is_sorted_iterator &&
    B::is_sorted_iterator;
  __item_t__ __item__ () const { return __item_t__ (*a, *b); }
  __item_t__ __item_at__ (unsigned i) const { return __item_t__ (a[i], b[i]); }
  bool __more__ () const { return a && b; }
  unsigned __len__ () const { return hb_min (a.len (), b.len ()); }
  void __next__ () { ++a; ++b; }
  void __forward__ (unsigned n) { a += n; b += n; }
  void __prev__ () { --a; --b; }
  void __rewind__ (unsigned n) { a -= n; b -= n; }
  hb_zip_iter_t __end__ () const { return hb_zip_iter_t (a.end (), b.end ()); }
  bool operator != (const hb_zip_iter_t& o) const
  { return a != o.a || b != o.b; }

  private:
  A a;
  B b;
};
struct
{
  template <typename A, typename B,
	    hb_requires (hb_is_iterable (A) && hb_is_iterable (B))>
  hb_zip_iter_t<hb_iter_type<A>, hb_iter_type<B>>
  operator () (A& a, B &b) const
  { return hb_zip_iter_t<hb_iter_type<A>, hb_iter_type<B>> (hb_iter (a), hb_iter (b)); }
}
HB_FUNCOBJ (hb_zip);

/* hb_enumerate */

template <typename Iter,
	 hb_requires (hb_is_iterator (Iter))>
struct hb_enumerate_iter_t :
  hb_iter_t<hb_enumerate_iter_t<Iter>,
	    hb_pair_t<unsigned, typename Iter::item_t>>
{
  hb_enumerate_iter_t (const Iter& it) : i (0), it (it) {}

  typedef hb_pair_t<unsigned, typename Iter::item_t> __item_t__;
  static constexpr bool is_random_access_iterator = Iter::is_random_access_iterator;
  static constexpr bool is_sorted_iterator = true;
  __item_t__ __item__ () const { return __item_t__ (+i, *it); }
  __item_t__ __item_at__ (unsigned j) const { return __item_t__ (i + j, it[j]); }
  bool __more__ () const { return bool (it); }
  unsigned __len__ () const { return it.len (); }
  void __next__ () { ++i; ++it; }
  void __forward__ (unsigned n) { i += n; it += n; }
  void __prev__ () { --i; --it; }
  void __rewind__ (unsigned n) { i -= n; it -= n; }
  hb_enumerate_iter_t __end__ () const
  {
    if (is_random_access_iterator)
      return *this + this->len ();
    /* Above expression loops twice. Following loops once. */
    auto it = *this;
    while (it) ++it;
    return it;
  }
  bool operator != (const hb_enumerate_iter_t& o) const
  { return i != o.i || it != o.it; }

  private:
  unsigned i;
  Iter it;
};
struct
{
  template <typename Iterable,
	    hb_requires (hb_is_iterable (Iterable))>
  hb_enumerate_iter_t<hb_iter_type<Iterable>>
  operator () (Iterable&& it) const
  { return hb_enumerate_iter_t<hb_iter_type<Iterable>> (hb_iter (it)); }
}
HB_FUNCOBJ (hb_enumerate);

/* hb_apply() */

template <typename Appl>
struct hb_apply_t
{
  hb_apply_t (Appl a) : a (a) {}

  template <typename Iter,
	    hb_requires (hb_is_iterator (Iter))>
  void operator () (Iter it)
  {
    for (; it; ++it)
      (void) hb_invoke (a, *it);
  }

  private:
  Appl a;
};
struct
{
  template <typename Appl> hb_apply_t<Appl>
  operator () (Appl&& a) const
  { return hb_apply_t<Appl> (a); }

  template <typename Appl> hb_apply_t<Appl&>
  operator () (Appl *a) const
  { return hb_apply_t<Appl&> (*a); }
}
HB_FUNCOBJ (hb_apply);

/* hb_iota() */

template <typename T, typename S>
struct hb_iota_iter_t :
  hb_iter_t<hb_iota_iter_t<T, S>, T>
{
  hb_iota_iter_t (T start, T end, S step) : v (start), end (end_for (start, end, step)), step (step) {}

  typedef T __item_t__;
  static constexpr bool is_random_access_iterator = true;
  static constexpr bool is_sorted_iterator = true;
  __item_t__ __item__ () const { return v; }
  __item_t__ __item_at__ (unsigned j) const { return v + j * step; }
  bool __more__ () const { return v != end; }
  unsigned __len__ () const { return (end - v) / step; }
  void __next__ () { v += step; }
  void __forward__ (unsigned n) { v += n * step; }
  void __prev__ () { v -= step; }
  void __rewind__ (unsigned n) { v -= n * step; }
  hb_iota_iter_t __end__ () const { hb_iota_iter_t (end, end, step); }
  bool operator != (const hb_iota_iter_t& o) const
  { return v != o.v || end != o.end || step != o.step; }

  private:
  static inline T end_for (T start, T end, S step)
  {
    auto res = (end - start) % step;
    if (!res)
      return end;
    end += step - res;
    return end;
  }

  private:
  T v;
  T end;
  S step;
};
struct
{
  template <typename T = unsigned> hb_iota_iter_t<T, unsigned>
  operator () (T end = (unsigned) -1) const
  { return hb_iota_iter_t<T, unsigned> (0, end, 1u); }

  template <typename T, typename S = unsigned> hb_iota_iter_t<T, S>
  operator () (T start, T end, S&& step = 1u) const
  { return hb_iota_iter_t<T, S> (start, end, step); }
}
HB_FUNCOBJ (hb_iota);


/* hb_sink() */

template <typename Sink>
struct hb_sink_t
{
  hb_sink_t (Sink s) : s (s) {}

  template <typename Iter,
	    hb_requires (hb_is_iterator (Iter))>
  void operator () (Iter it)
  {
    for (; it; ++it)
      s << *it;
  }

  private:
  Sink s;
};
struct
{
  template <typename Sink> hb_sink_t<Sink>
  operator () (Sink&& s) const
  { return hb_sink_t<Sink> (s); }

  template <typename Sink> hb_sink_t<Sink&>
  operator () (Sink *s) const
  { return hb_sink_t<Sink&> (*s); }
}
HB_FUNCOBJ (hb_sink);

/* hb-drain: hb_sink to void / blackhole / /dev/null. */

struct
{
  template <typename Iter,
	    hb_requires (hb_is_iterator (Iter))>
  void operator () (Iter it) const
  {
    for (; it; ++it)
      (void) *it;
  }
}
HB_FUNCOBJ (hb_drain);

/* hb_unzip(): unzip and sink to two sinks. */

template <typename Sink1, typename Sink2>
struct hb_unzip_t
{
  hb_unzip_t (Sink1 s1, Sink2 s2) : s1 (s1), s2 (s2) {}

  template <typename Iter,
	    hb_requires (hb_is_iterator (Iter))>
  void operator () (Iter it)
  {
    for (; it; ++it)
    {
      const auto &v = *it;
      s1 << v.first;
      s2 << v.second;
    }
  }

  private:
  Sink1 s1;
  Sink2 s2;
};
struct
{
  template <typename Sink1, typename Sink2> hb_unzip_t<Sink1, Sink2>
  operator () (Sink1&& s1, Sink2&& s2) const
  { return hb_unzip_t<Sink1, Sink2> (s1, s2); }

  template <typename Sink1, typename Sink2> hb_unzip_t<Sink1&, Sink2&>
  operator () (Sink1 *s1, Sink2 *s2) const
  { return hb_unzip_t<Sink1&, Sink2&> (*s1, *s2); }
}
HB_FUNCOBJ (hb_unzip);


/* hb-all, hb-any, hb-none. */

struct
{
  template <typename Iterable,
	    typename Pred = decltype ((hb_identity)),
	    typename Proj = decltype ((hb_identity)),
	    hb_requires (hb_is_iterable (Iterable))>
  bool operator () (Iterable&& c,
		    Pred&& p = hb_identity,
		    Proj&& f = hb_identity) const
  {
    for (auto it = hb_iter (c); it; ++it)
      if (!hb_match (hb_forward<Pred> (p), hb_get (hb_forward<Proj> (f), *it)))
	return false;
    return true;
  }
}
HB_FUNCOBJ (hb_all);
struct
{
  template <typename Iterable,
	    typename Pred = decltype ((hb_identity)),
	    typename Proj = decltype ((hb_identity)),
	    hb_requires (hb_is_iterable (Iterable))>
  bool operator () (Iterable&& c,
		    Pred&& p = hb_identity,
		    Proj&& f = hb_identity) const
  {
    for (auto it = hb_iter (c); it; ++it)
      if (hb_match (hb_forward<Pred> (p), hb_get (hb_forward<Proj> (f), *it)))
	return true;
    return false;
  }
}
HB_FUNCOBJ (hb_any);
struct
{
  template <typename Iterable,
	    typename Pred = decltype ((hb_identity)),
	    typename Proj = decltype ((hb_identity)),
	    hb_requires (hb_is_iterable (Iterable))>
  bool operator () (Iterable&& c,
		    Pred&& p = hb_identity,
		    Proj&& f = hb_identity) const
  {
    for (auto it = hb_iter (c); it; ++it)
      if (hb_match (hb_forward<Pred> (p), hb_get (hb_forward<Proj> (f), *it)))
	return false;
    return true;
  }
}
HB_FUNCOBJ (hb_none);

/*
 * Algorithms operating on iterators.
 */

template <typename C, typename V,
	  hb_requires (hb_is_iterable (C))>
inline void
hb_fill (C& c, const V &v)
{
  for (auto i = hb_iter (c); i; i++)
    *i = v;
}

template <typename S, typename D>
inline void
hb_copy (S&& is, D&& id)
{
  hb_iter (is) | hb_sink (id);
}


#endif /* HB_ITER_HH */
