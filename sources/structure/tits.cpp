/*!
\file
\brief Implementation of the classes TitsGroup and TitsElt.

  This module contains an implementation of a slight variant of the
  Tits group (also called extended Weyl group) as defined in J. Tits,
  J. of Algebra 4 (1966), pp. 96-116.

  The slight variant is that we include all elements of order two in the
  torus, instead of just the subgroup generated by the \f$m_\alpha\f$ (denoted
  \f$h_\alpha\f$ in Tits' paper.) Tits' original group may be defined by
  generators \f$\sigma_\alpha\f$ for \f$\alpha\f$ simple, subject to the braid
  relations and to \f$\sigma_\alpha^2= m_\alpha\f$; to get our group we just
  add a basis of elements of $H(2)$ as additional generators, and express the
  \f$m_\alpha\f$ in this basis. This makes for a simpler implementation, where
  torus parts are just elements of the $Z/2Z$-vector space $H(2)$.

  On a practical level, because the \f$\sigma_\alpha\f$ satisfy the braid
  relations, any element of the Weyl group has a canonical lifting in the Tits
  group; so we may uniquely represent any element of the Tits group as a pair
  $(t,w)$, with $t$ in $T(2)$ and $w$ in $W$ (the latter representing the
  canonical lift \f$\sigma_w\f$. The multiplication rules have to be
  thoroughly changed, though, to take into account the new relations.

  We have not tried to be optimally efficient here, as it is not
  expected that Tits computations will be significant computationally.
*/
/*
  This is tits.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#include "tits.h"

#include "lattice.h"
#include "rootdata.h"
#include "setutils.h"

#include "complexredgp.h"
#include "realredgp.h"
#include "subquotient.h"

#include <cassert>

namespace atlas {

namespace tits {

namespace {

std::vector<gradings::Grading> compute_square_classes
  (const complexredgp::ComplexReductiveGroup& G);

} // |namespace|

/****************************************************************************

        Chapter I -- The |TorusElement| and |GlobalTitsGroup| classes

*****************************************************************************/

// make $\exp(2\pi i r)$, doing affectively *2, then reducing modulo $2X_*$
TorusElement::TorusElement(const latticetypes::RatWeight r)
  : repr(r.numerator()*2,r.denominator())
{ repr.normalize(); // maybe cancels the factor 2 in numerator from denominator
  unsigned int d=2*repr.denominator(); // we reduce modulo $2\Z^rank$
  latticetypes::LatticeElt& num=repr.numerator();
  for (size_t i=0; i<num.size(); ++i)
    num[i] = intutils::remainder(num[i],d);
}

TorusElement TorusElement::operator +(const TorusElement& t) const
{
  TorusElement result(repr + t.repr,0); // raw constructor
  int d=2*result.repr.denominator(); // we shall reduce modulo $2\Z^rank$
  latticetypes::LatticeElt& num=result.repr.numerator();
  for (size_t i=0; i<num.size(); ++i)
    if (num[i] >= d) // correct if |result.repr| in interval $[2,4)$
      num[i] -=d;
  return result;
}

TorusElement TorusElement::operator -(const TorusElement& t) const
{
  TorusElement result(repr - t.repr,0); // raw constructor
  int d=2*result.repr.denominator(); // we shall  reduce modulo $2\Z^rank$
  latticetypes::LatticeElt& num=result.repr.numerator();
  for (size_t i=0; i<num.size(); ++i)
    if (num[i]<0) // correct if |result.repr| in interval $(-2,0)$
      num[i] +=d;
  return result;
}

TorusElement& TorusElement::operator+=(TorusPart v)
{
  for (size_t i=0; i<v.size(); ++i)
    if (v[i])
    {
      if ((unsigned long)repr.numerator()[i]<repr.denominator())
	repr.numerator()[i]+=repr.denominator(); // add 1/2 to coordinate
      else
	repr.numerator()[i]-=repr.denominator(); // subtract 1/2
    }
  return *this;
}

GlobalTitsGroup::GlobalTitsGroup(const complexredgp::ComplexReductiveGroup& G)
  : weyl::TwistedWeylGroup(G.twistedWeylGroup())
  , root_datum(G.rootDatum())
  , alpha_v(G.semisimpleRank())
  , square_class_gen(compute_square_classes(G))
  , theta_v(G.distinguished().transposed())
{
  for (size_t i=0; i<alpha_v.size(); ++i) // reduce vectors mod 2
    alpha_v[i]=latticetypes::SmallBitVector(root_datum.simpleCoroot(i));
}

int // length change in $\{-1,0,+1\}$, half that of change for |x.tw()|
GlobalTitsGroup::cross(weyl::Generator s, GlobalTitsElement& x) const
{
  bool c = x.t.negative_at(simple_root(s)); // compact xor ($\alpha_s$ complex)
  int d=twistedConjugate(x.w,s); // $d$ is length change in $W$ of |tw|
  if (c != (d==0)) // length change iff $\alpha_s$ complex; true <=> compact
    add(alpha_v[s],x);
  return d/2;
}

GlobalTitsElement GlobalTitsGroup::Cayley
  (weyl::Generator s, GlobalTitsElement x) const
{
  leftMult(x.w,s); return x;
}

namespace {

/*
 The |square_class_gen| field of a |GlobalTitsGroup| contains a list of
 generators of gradings of the simple roots that generate the square classes,
 if translated into central |TorusElement| values by taking the combination
 $c$ of fund. coweights selected by the grading, and then applying the map
 $c\mapsto\exp(2\pi i c)$; the function |compute_square_classes| computes it.
 As representative torus element for the class one can take $t=\exp(\pi i c)$.

 Actually what is graded is the set of $\delta$-fixed root vectors, and the
 grading is that for the strong involution $t.\delta$ relative to the grading
 of $\delta$, which is just $v(t)=(-1)^{\<v,c>}$ at root vector $v$. But since
 $c$ must be $\delta$-stable to define a valid square of a strong involution,
 we certainly have $\<(1+\delta)v,c>\in2\Z$ for all root vectors $v$; thus the
 grading is trivial on the $1+\delta$-image of the root lattice, and it
 suffices to record the grading on the $\delta$-fixed simple roots.

 The list is formed of generators of the quotient of all possible gradings of
 those simple roots by the subgroup of gradings coming from $\delta$-fixed
 elements of $X_*$ (since gradings in the same coset for that subgroup define
 the same square class). We get a basis of the subgroup from |tori::plusBasis|
 and get gradings from it by taking scalar products with simple roots.

 The generators can all be taken to be canonical basis elements (gradings with
 exactly one bit set) at $\delta$-fixed simple roots. Once the subgroup to
 quotient by is constructed, we can simply take those canonical basis vectors
 not in the "support" of the subgroup (so that reduction would be trivial).
 */
std::vector<gradings::Grading> compute_square_classes
  (const complexredgp::ComplexReductiveGroup& G)
{
  const rootdata::RootDatum& rd = G.rootDatum();
  const latticetypes::LatticeMatrix& delta = G.distinguished();
  const weyl::Twist& twist = G.twistedWeylGroup().twist();

  size_t r= rd.rank();
  assert(delta.numRows()==r);
  assert(delta.numColumns()==r);

  latticetypes::LatticeMatrix roots(0,r);
  bitset::RankFlags fixed;
  for (size_t i=0; i<rd.semisimpleRank(); ++i)
    if (twist[i]==i)
    {
      fixed.set(i);
      roots.add_row(rd.simpleRoot(i));
    }

  latticetypes::BinaryMap A(lattice::eigen_lattice(delta.transposed(),1));
  latticetypes::SmallSubspace Vplus(A); // mod subgroup, in $X_*$ coordinates
  latticetypes::BinaryMap to_grading(roots);
  Vplus.apply(to_grading); // convert to grading coordinates

  bitset::RankFlags supp = Vplus.support(); // pivot positions
  supp.complement(roots.numRows()); // non-pivot positions among fixed ones
  supp.unslice(fixed);  // bring bits back to the simple-root positions

  std::vector<gradings::Grading> result; result.reserve(supp.count());

  for (bitset::RankFlags::iterator it=supp.begin(); it(); ++it)
  {
    result.push_back(gradings::Grading());
    result.back().set(*it);
  }

  return result;
}

} // |namespace|


/****************************************************************************

        Chapter II -- The TitsGroup class

*****************************************************************************/

/*!
  Constructs the Tits group corresponding to the root datum |rd|, and
  the fundamental involution |d|.
*/
TitsGroup::TitsGroup(const rootdata::RootDatum& rd,
		     const weyl::WeylGroup& W,
		     const latticetypes::LatticeMatrix& d,
		     const weyl::Twist& twist)
  : weyl::TwistedWeylGroup(W,twist)
  , d_rank(rd.rank())
  , d_simpleRoot(rd.semisimpleRank())   // set number of vectors, but not yet
  , d_simpleCoroot(rd.semisimpleRank()) // their size (which will be |d_rank|)
  , d_involution(d.transposed())
{
  for (size_t i = 0; i<rd.semisimpleRank(); ++i) // reduce vectors mod 2
  {
    d_simpleRoot[i]  =latticetypes::SmallBitVector(rd.simpleRoot(i));
    d_simpleCoroot[i]=latticetypes::SmallBitVector(rd.simpleCoroot(i));
  }
}


/*!
  Constructs the Tits group corresponding to the adjoint group of type given
  by |Cartan_matrix| and distinguished involution given by |twist|
*/
TitsGroup::TitsGroup(const latticetypes::LatticeMatrix& Cartan_matrix,
		     const weyl::WeylGroup& W,
		     const weyl::Twist& twist)
  : weyl::TwistedWeylGroup(W,twist)
  , d_rank(Cartan_matrix.numRows())
  , d_simpleRoot()
  , d_simpleCoroot()
  , d_involution(d_rank)   // square matrix, initially zero
{
  d_simpleRoot.reserve(d_rank); d_simpleCoroot.reserve(d_rank);
  for (size_t i=0; i<d_rank; ++i)
  {
    d_simpleRoot.push_back(latticetypes::SmallBitVector(d_rank,i)); // $e_i$
    d_simpleCoroot.push_back(latticetypes::SmallBitVector
			     (Cartan_matrix.column(i))); // adjoint coroot
    d_involution.set(i,twist[i]); // (transpose of) |twist| permutation matrix
  }
}


// Switching between left and right torus parts is a fundamental tool.

/*!
  \brief find torus part $x'$ so that $x.w=w.x'$

  Algorithm: for simple reflections this is |reflect|; repeat left-to-right

  Note: a more sophisticated algorithm would involve precomputing the
  conjugation matrices for the various pieces of w. Don't do it unless it
  turns out to really matter.
*/
TorusPart TitsGroup::push_across(TorusPart x, const weyl::WeylElt& w) const
{
  weyl::WeylWord ww=weylGroup().word(w);

  for (size_t i = 0; i < ww.size(); ++i)
    reflect(x,ww[i]);

  return x;
}

// find torus part $x'$ so that $w.x=x'.w$; inverse to |push_across|
TorusPart TitsGroup::pull_across(const weyl::WeylElt& w, TorusPart x) const
{
  weyl::WeylWord ww=weylGroup().word(w);
  for (size_t i=ww.size(); i-->0; )
    reflect(x,ww[i]);
  return x;
}


/*!
  \brief Left multiplication of |a| by the canonical generator \f$\sigma_s\f$.

  This is the basic case defining the group structure in the Tits group (since
  left-multiplication by an element of $T(2)$ just adds to the torus part).

  Algorithm: This is surprisingly simple: multiplying by \f$\sigma_s\f$ just
  amounts to reflecting the torus part through |s|, then left-multiplying the
  Weyl group part $w$ by |s| in the Weyl group, and if the length goes down in
  the latter step, add a factor of \f$(\sigma_s)^2=m_s\f$ to the torus part.
  (To see this, write in the length-decreasing case $w=s.w'$ with $w'$
  reduced; then \f$\sigma_s\sigma_w=m_s\sigma_{w'}\f$, so we need to add $m_s$
  to the reflected left torus part in that case.

  The upshot is a multiplication algorithm almost as fast as in the Weyl group!
*/
void TitsGroup::sigma_mult(weyl::Generator s,TitsElt& a) const
{
  reflect(a.d_t,s); // commute (left) torus part with $\sigma_s$
  if (weylGroup().leftMult(a.d_w,s)<0) // on length decrease
    left_add(d_simpleCoroot[s],a);     // adjust torus part
}

void TitsGroup::sigma_inv_mult(weyl::Generator s,TitsElt& a) const
{
  reflect(a.d_t,s); // commute (left) torus part with $\sigma_s$
  if (weylGroup().leftMult(a.d_w,s)>0) // on length increase
    left_add(d_simpleCoroot[s],a);     // adjust torus part
}


/*!
  \brief Right multiplication of |a| by the canonical generator \f$sigma_s\f$.

  Algorithm: similar to above, but omitting the |reflect| (since the torus
  part is at the left), and using |weyl::mult| instead of |weyl::leftMult|,
  and |right_add| to add the possible contribution $m_s$ instead of
  |left_add|; the latter implicitly involves a call to |pull_across|.
*/
void TitsGroup::mult_sigma(TitsElt& a, weyl::Generator s) const
{
// |WeylGroup::mult| multiplies $w$ by $s$, returns sign of the length change
  if (weylGroup().mult(a.d_w,s)<0)  // on length decrease adjust torus part
    right_add(a,d_simpleCoroot[s]);
}

void TitsGroup::mult_sigma_inv(TitsElt& a, weyl::Generator s) const
{
  if (weylGroup().mult(a.d_w,s)>0) // on length increase adjust torus part
    right_add(a,d_simpleCoroot[s]);
}


/*!
  \brief Product of general Tits group elements

  Algorithm: The algorithm is to multiply the second factors successively by
  the generators in a reduced expression for |a.w()|, then left-add |a.t()|.

  Since we have torus parts on the left, we prefer left-multiplication here.
*/
TitsElt TitsGroup::prod(const TitsElt& a, TitsElt b) const
{
  weyl::WeylWord ww=weylGroup().word(a.w());

  // first incorporate the Weyl group part
  for (size_t i = ww.size(); i-->0; )
    sigma_mult(ww[i],b);

  // and finally the torus part
  left_add(a.d_t,b);
  return b;
}

// here the we basically do Weyl group action at torus side, including twist
latticetypes::BinaryMap
TitsGroup::involutionMatrix(const weyl::WeylWord& ww) const
{
  latticetypes::BinaryMap result(rank(),0);
  for (size_t i=0; i<rank(); ++i)
  {
    latticetypes::SmallBitVector v(rank(),i); // basis vector $e_i$
    // act by letters of |ww| from left to right (because transposed action)
    for (size_t j=0; j<ww.size(); ++j)
      reflect(v,ww[j]);
    result.addColumn(twisted(v)); // finally twist
  }
  return result;
}






/*
 *
 *				BasedTitsGroup
 *
 */

BasedTitsGroup::BasedTitsGroup(const complexredgp::ComplexReductiveGroup& G,
			       gradings::Grading base_grading)
  : my_Tits_group(NULL) // no ownership in this case
  , Tg(G.titsGroup())
  , grading_offset(base_grading)
  , rs(G.rootDatum())
{
}

// Based Tits group for the adjoint group
BasedTitsGroup::BasedTitsGroup(const complexredgp::ComplexReductiveGroup& G)
  : my_Tits_group(new tits::TitsGroup(G.rootDatum().cartanMatrix(),
				      G.weylGroup(),
				      G.twistedWeylGroup().twist()))
  , Tg(*my_Tits_group)
  , grading_offset()
  , rs(G.rootDatum())
{ // make all imaginary simple roots noncompact
  for (unsigned i=0; i<G.semisimpleRank(); ++i)
    grading_offset.set(i,Tg.twisted(i)==i);
}

// Based Tits group for the adjoint group
BasedTitsGroup::BasedTitsGroup(const complexredgp::ComplexReductiveGroup& G,
			       tags::DualTag)
  : my_Tits_group(new tits::TitsGroup(G.rootDatum().cartanMatrix().transposed(),
				      G.weylGroup(),
				      G.twistedWeylGroup().dual_twist()))
  , Tg(*my_Tits_group)
  , grading_offset()
  , rs(G.dualRootDatum())
{ // make all imaginary simple roots noncompact
  for (unsigned i=0; i<G.semisimpleRank(); ++i)
    grading_offset.set(i,Tg.twisted(i)==i);
}

void BasedTitsGroup::basedTwistedConjugate
  (tits::TitsElt& a, const weyl::WeylWord& w) const
{
  for (size_t i=0; i<w.size(); ++i)
    basedTwistedConjugate(a,w[i]);
}

void BasedTitsGroup::basedTwistedConjugate
  (const weyl::WeylWord& w, tits::TitsElt& a) const
{
  for (size_t i=w.size(); i-->0; )
    basedTwistedConjugate(a,w[i]);
}


/* Inverse Cayley transform is more delicate than Cayley transform, since the
   torus part has been modularly reduced when passing from an the involution
   $\theta$ to $\theta_\alpha$. While before the Cayley transform the value
   ($\pm1$) of each $\theta$-stable weight on the torus part was well defined,
   the modular reduction retains only the values for $\theta_\alpha$-stable
   weights. When going back, the values of all $\theta$-stable weights must be
   defined again, but some might have changed by the reduction. It turns out
   that our reconstruction of the torus part is valid if and only if the
   evaluation of the simple root $\alpha=\alpha_i$ through which we transform
   (and which becomes imaginary) is $-1$, so that $\alpha$ becomes noncompact.
   Indeed, if the transformation was type I, then the value of this root is
   already determined by the values of weights fixed by the new involution:
   the modular reduction was by $m_\alpha$, for which $\alpha$ evaluates to
   $+1$, and which forms the difference between the torus parts of the two
   elements which the same value of the Cayley transform, from which we have
   to make a choice anyway. If the transformation was type II, then the
   sublattice of $\theta$-stable weights is the direct sum of the
   $\theta_\alpha$-stable weights and the multiples of $\alpha$, and lifting
   the torus part means determining the evaluation of $\alpha$ at it; since
   there are always two possible lifts, one of them makes $\alpha$ noncompact.
 */
void BasedTitsGroup::inverse_Cayley_transform
  (tits::TitsElt& a, size_t i,
   const latticetypes::SmallSubspace& mod_space) const
{
  Tg.sigma_inv_mult(i,a); // set |a| to $\sigma_i^{-1}.a$
  if (not simple_grading(a,i)) // $\alpha_i$ should not become compact!
  {
    // we must find a vector in |mod_space| affecting grading at $\alpha_i$
    for (size_t j=0; j<mod_space.dimension(); ++j) // a basis vector will do
      if (Tg.dual_m_alpha(i).dot(mod_space.basis(j)))
      { // scalar product true means grading is affected: we found it
	Tg.left_add(mod_space.basis(j),a); // adjust |a|'s torus part
	break;
      }

    assert(simple_grading(a,i)); // the problem must now be corrected
  }
}


tits::TitsElt BasedTitsGroup::twisted(const tits::TitsElt& a) const
{
  tits::TitsElt result(Tg,Tg.twisted(Tg.left_torus_part(a)));
  weyl::WeylWord ww=Tg.word(a.w());
  for (size_t i=0; i<ww.size(); ++i)
  {
    weyl::Generator s=Tg.twisted(ww[i]);
    Tg.mult_sigma(result,s);
    if (grading_offset[s]) // $s$ noncompact imaginary: add $m_\alpha$
      Tg.right_add(result,Tg.m_alpha(s));
  }
  return result;
}

bool BasedTitsGroup::grading(tits::TitsElt a, rootdata::RootNbr alpha) const
{
  if (not rs.isPosRoot(alpha))
    alpha=rs.rootMinus(alpha);

  assert(rs.isPosRoot(alpha));
  size_t i; // declare outside loop to allow inspection of final value
  while (alpha!=rs.simpleRootNbr(i=rs.find_descent(alpha)))
  {
    basedTwistedConjugate(a,i);
    rs.simple_reflect_root(alpha,i);
  }

  return simple_grading(a,i);
}

/* The method |naive_seed| attempts to get an initial Tits group element, for
   a KGB construction for the real form |rf| starting at Cartan class |cn|, by
   extracting the necessary information fom the |Fiber| object associated to
   the Cartan class |cn|, and lifting that information from the level of the
   fiber group back to the level of torus parts. But as the name indicates,
   the result is not always good, as it fails to account for the different
   grading choices involved in identifying the (weak) real form in the fiber
   and in the KGB construction. In the fiber construction, the action used to
   partition the fiber group according to real forms uses a grading of the
   imaginary roots for the twisted involution that makes all simple-imaginary
   ones noncompact. In the KGB construction (i.e., in our |BasedTitsGroup|) a
   grading is chosen only at the distinguished involution, and only of the
   simple roots that are imaginary for that involution; it depends on the
   square class of the real form. The bitvector |v| below is zero for some
   strong involution |si| in same square class as |rf| at Cartan class |cn|;
   the result will be correct if and only if the null torus part |x| defines
   (by |BasedTitsGroup::grading|) the same grading of the (simple) imaginary
   roots for Cartan class |cn| as |si| does (through |Fiber::class_base|).

   The only case where one can rely on that to be true is for the fundamental
   Cartan (|cn==0|), if our |BasedTitsGroup| was extracted as base object from
   an |EnrichedTitsGroup| (the latter being necessarily constructed through
   |EnrichedTitsGroup::for_square_class|), because in that case the
   |BasedTitsGroup::grading_offset| field was actually computed from de
   grading defined by an element |si| in the fundamental fiber. In the general
   case all bets are off: the real form of |si| need not even be the same one
   as the one corresponding to |BasedTitsGroup::grading_offset|.

   The main reason for leaving this (unused) method in the code is that it
   illustrates how to get the group morphism from the fiber group to T(2).
 */
tits::TitsElt BasedTitsGroup::naive_seed
 (complexredgp::ComplexReductiveGroup& G,
  realform::RealForm rf, size_t cn) const
{
  // locate fiber, weak and strong real forms, and check central square class
  const cartanclass::Fiber& f=G.cartan(cn).fiber();
  cartanclass::adjoint_fiber_orbit wrf = G.real_form_part(rf,cn);
  cartanclass::StrongRealFormRep srf=f.strongRepresentative(wrf);
  assert(srf.second==f.central_square_class(wrf));
  // the |grading_offset| of our |BasedTitsGroup| gives the square class base

  // now lift strong real form from fiber group to a torus part in |result|
  latticetypes::SmallBitVector v(bitset::RankFlags(srf.first),f.fiberRank());
  tits::TorusPart x = f.fiberGroup().fromBasis(v);

  // right-multiply this torus part by canonical twisted involution for |cn|
  tits::TitsElt result(titsGroup(),x,G.twistedInvolution(cn));

  return result; // result should be reduced immediatly by caller
}

/* The method |grading_seed| attempts to correct the shortcomings of
   |naive_seed| by insisting on obtaining an element exhibiting a grading that
   corresponds to the real form |rf|. Thus no element is actually recovered
   from any fiber group, but rather a set of equations for the torus part is
   set up and solved. The equations come in two parts: a first section for
   establishing the correct coset in T(2) with respect to the "numerator"
   subgroup of the subquotient that defines th fiber group, and a second
   section for requiring the correct grading of the imaginary roots for the
   real form that is intended. Both parts are inhomogeneous linear equations,
   of which the left hand side (linear part) expresses a linear dependence on
   the torus part, and the right hand side (inhomogeneous part) describes the
   failure of the null torus part to satisfy the conditions (since these are
   equations over Z/2Z, there is no need to put in a minus sign).

   The equations of the first section are derived from our test of being in
   the proper coset for this square class (mentioned in kgb.cpp), namely that
   the Tits element |a| with this left torus part and canonical twisted
   involution for the Cartan satisfies $a*twisted(a)=e$ in our based twisted
   Tits group. The left hand side describes the action of the involution on
   torus parts, as provided by |Tg.involutionMatrix|. The right hand side is
   equal to $a*twisted(a)$ where |a| is the canonical lift of the twisted
   involution to the Tits group (i.e;, its torus part is null).

   The equations of the second section have as left hand sides the reductions
   modulo 2 of the simple-imaginary roots for the twisted involution,
   interpreted as Z/2Z-linear forms on torus parts, and as corresponding right
   hand side the difference between the desired grading of that root and the
   gradings of it defined by the element |a| above, with null torus part.

   It is not obvious that these equations actually have a solution, but since
   the traditional KGB construction in fact produces such elements, we assert
   that one exists. The solution is only meaningful modulo the "denominator"
   subgroup of the subquotient that defines th fiber group, so the result
   returned should be reduced modulo that subgroup by the caller.

   If grading seeds are determined for different Cartan classes, there is no
   guarantee that chosen solutions will belong to the same strong real form.
   Therefore this method should only be called when only one seed is needed.
 */
tits::TitsElt BasedTitsGroup::grading_seed
  (complexredgp::ComplexReductiveGroup& G,
   realform::RealForm rf, size_t cn) const
{
  // locate fiber and weak real form
  const cartanclass::Fiber& f=G.cartan(cn).fiber();
  cartanclass::adjoint_fiber_orbit wrf = G.real_form_part(rf,cn);
  const weyl::TwistedInvolution& tw = G.twistedInvolution(cn);

  // get an element lying over the canonical twisted involution for |cn|
  tits::TitsElt a(Tg,tw); // trial element with null torus part

  // get the grading of the imaginary root system given by the element |a|
  gradings::Grading base_grading;
  for (size_t i=0; i<f.imaginaryRank(); ++i)
    base_grading.set(i,grading(a,f.simpleImaginary(i)));

  // get the grading of the same system given by chosen representative of |wrf|
  gradings::Grading form_grading = f.grading(f.weakReal().classRep(wrf));
  /* the difference between |base_grading| and |form_grading| will have to be
     compensated by setting an appropriate torus part for |a| */

  // now prepare equations for coset for "fiber numerator" group of torus part
  Tg.mult(a,twisted(a)); // now |a.t()| gives inhomogenous part for equations

  latticetypes::BinaryEquationList eqns;  // equations for our seed
  eqns.reserve(G.rank()+f.imaginaryRank()); // for coset + grading

  latticetypes::BinaryMap refl = Tg.involutionMatrix(G.weylGroup().word(tw));

  // coset equations
  for (size_t i=0; i<G.rank(); ++i)
  {
    latticetypes::SmallBitVector lhs = refl.row(i);
    lhs.flip(i); // left hand side is row $i$ of $TorusPartInvolution-1$
    eqns.push_back(latticetypes::make_equation(lhs,Tg.left_torus_part(a)[i]));
  }
  // grading equations
  for (size_t i=0; i<f.imaginaryRank(); ++i)
  {
    latticetypes::BinaryEquation equation =
      latticetypes::BinaryEquation(G.rootDatum().root(f.simpleImaginary(i)));
    equation.pushBack((base_grading^form_grading)[i]);
    eqns.push_back(equation);
  }

  // solve, and tack a solution |x| to the left of |a|.
  tits::TorusPart x(G.rank());
#ifndef NDEBUG
  bool success=bitvector::firstSolution(x,eqns);
  assert(success);
#else
  bitvector::firstSolution(x,eqns);
#endif

  tits::TitsElt seed(Tg,x,tw); // $x.\sigma_w$

#ifndef NDEBUG
  // double-check that we have found an element with require properties

  tits::TitsElt check=seed; Tg.mult(check,twisted(check));
  assert(check==tits::TitsElt(Tg)); // we are in the proper coset

  for (size_t i=0; i<f.imaginaryRank(); ++i)
    assert(grading(seed,f.simpleImaginary(i))==form_grading[i]); // wrf OK
#endif

  return seed;  // result should be reduced immediatly by caller
}

/*! \brief Returns the grading offset for the base real form of the square
 class (coset in adjoint fiber group) |csc|; |fund| and |rs| are corresponding
 values. |fund| must be a fundamental fiber, in order that restricting grading
 to simple roots suffice to determine the real form, or even the square class
 */
gradings::Grading
square_class_grading_offset(const cartanclass::Fiber& fund,
			    cartanclass::square_class csc,
			    const rootdata::RootSystem& rs)
{
  rootdata::RootSet rset = fund.compactRoots(fund.class_base(csc));
  return cartanclass::restrictGrading(rset,rs.simpleRootList())// restrict
    .complement(rs.rank());// and complement with respec to to simple roots

}

EnrichedTitsGroup::EnrichedTitsGroup(const realredgp::RealReductiveGroup& GR)
  : BasedTitsGroup(GR.complexGroup(),
		   square_class_grading_offset(GR.complexGroup().fundamental(),
					       GR.square_class(),
					       GR.rootDatum()))
  , srf(GR.complexGroup().fundamental().strongRepresentative(GR.realForm()))
{}


/* In this final and most elaborate seeding function, which is also the most
   reliable one, we stoop down to simulating the KGB construction back from
   the fundamental fiber to the one for which we try to find a seed, and to
   try all the representatives in the fundamental fiber of the strong real
   form, until finding one that, along the chosen path of cross actions and
   Cayley transforms, proves to be suited for every necessary Cayley transform
   (making the simple root involved noncompact).
 */
tits::TitsElt EnrichedTitsGroup::backtrack_seed
 (const complexredgp::ComplexReductiveGroup& G,
  realform::RealForm rf, size_t cn) const
{
  const tits::TitsGroup& Tg= titsGroup();

  const weyl::TwistedInvolution& tw=G.twistedInvolution(cn);

  rootdata::RootSet rset;
  weyl::WeylWord cross;
  complexredgp::Cayley_and_cross_part(rset,cross,tw,G.rootDatum(),Tg);

  /* at this point we can get from the fundamental fiber to |tw| by first
     applying cross actions according to |cross|, and then applying Cayley
     transforms in the strongly orthogonal set |Cayley|.
  */

  // transform strong orthogonal set |Cayley| back to distinguished involution
  rootdata::RootList Cayley(rset.begin(),rset.end()); // convert to |RootList|
  for (size_t i=0; i<Cayley.size(); ++i)
    for (size_t j=cross.size(); j-->0; )
      G.rootDatum().simple_reflect_root(Cayley[i],cross[j]);

  /* at this point we can get from the fundamental fiber to |tw| by first
     applying Cayley transforms in the strongly orthogonal set |Cayley|, and
     then applying cross actions according to |cross|
  */

  /* Now find an element in the chosen strong real form at the fundamental
     fiber, that has noncompact grading on all the roots of |Cayley| (which
     are imaginary for $\delta$)
   */
  tits::TitsElt result(Tg);

  const cartanclass::Fiber& fund=G.fundamental();
  const partition::Partition& srp = fund.strongReal(square());
  for (unsigned long x=0; x<srp.size(); ++x)
    if (srp(x)==srp(f_orbit()))
    {
      latticetypes::SmallBitVector v
	(static_cast<bitset::RankFlags>(x),fund.fiberRank());
      tits::TorusPart t = fund.fiberGroup().fromBasis(v);
      for (size_t i=0; i<Cayley.size(); ++i)
	if (is_compact(t,Cayley[i]))
	  goto again; // none of the |Cayley[i]| should be compact

      // if we get here, |t| is OK as torus part
      result = tits::TitsElt(titsGroup(),t); // pure torus part
      goto found;
    again: {}
    }
  assert(false); // getting here means none of the orbit elements is in order

found:

  /* Now we must apply the Cayley transforms and cross actions to |result|.
     However, Cayley transforms by non-simple roots are not implemented, and
     so we reorder the operations as in |Tg.involution_expr(tw)|, which gives
     the same cross actions, but interspersed with simple Cayley transforms.
   */

  // transform |result| via Cayley transforms and cross actions
  std::vector<signed char> dec=Tg.involution_expr(tw);
  for (size_t j=dec.size(); j-->0; )
    if (dec[j]>=0)
    {
      assert(simple_grading(result,dec[j])); // must be noncompact
      Cayley_transform(result,dec[j]);
    }
    else
      basedTwistedConjugate(result,~dec[j]);

  assert(result.tw()==tw);

  return result;  // result should be reduced immediatly by caller
}

} // namespace tits


} // namespace atlas
