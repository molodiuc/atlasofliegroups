/*!
\file
\brief Function and constant declarations for namespace lietype.
*/
/*
  This is lietype.h

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#ifndef LIETYPE_H  /* guard against multiple inclusions */
#define LIETYPE_H

#include "lietype_fwd.h"
#include "setutils.h"
#include "layout_fwd.h"  // not "layout.h", which must include us

#include "latticetypes_fwd.h"

namespace atlas {

/******** constant declarations **********************************************/

namespace lietype {
  /*!
  Used by the interaction in interactive_lietype.cpp to tell the user
  what input is expected for a Lie type.
  */
  const char* const typeLetters = "ABCDEFGT";

  /*!
  Used by the interaction in interactive_lietype.cpp to tell the user
  what input is expected for an inner class type.

  The letter "C" stands for "complex"; it goes with two adjacent equal
  Lie types, to say that the group has the corresponding complex
  simple factor.  (For example, a Lie type of A4.A4 and inner type C
  refers to SL(5,C).)

  The letter "c" stands for "compact"; it designates the inner class
  of forms in which rank(G) = rank(K) (so that there is a compact
  Cartan subgroup.)

  The letter "s" stands for "split," and designates the inner class
  containing the split real form of G.

  The letter "u" stands for "unequal rank," and designates an inner
  class of forms with no compact Cartan and (if possible) no split
  Cartan. This inner class is allowed only in A_n, n > 1, D_n, and
  E_6.  In types A, E6, and D_{2n+1} the unique unequal rank inner
  class is split, and the inner class designator "u" is then mapped to
  "s."  (This is important, because the code implementing the Cartan
  involution in lietype.cpp would not work in types A and E with the
  designator "u.")  In type D_{2n}, the inner class "u" corresponds to
  the Cartan involution exchanging the two simple roots at the
  branched end of the Dynkin diagram.  (In D_4 there are three
  involutions fitting this description.  The software picks just one
  of them.  Since the three differ by an outer automorphism of D_4,
  this loss of generality is harmless.)  The real forms in the inner
  class "u" for D_{2n} are the special orthogonal groups of signature
  (odd,odd).

  The letter "e" stands for "equal rank," and is mapped in every case to
  "c."
  */
  const char* const innerClassLetters = "Ccesu";

}

/******** function declarations **********************************************/

namespace lietype {

  bool checkRank(const TypeLetter&, size_t);

  LieType dual_type(LieType lt);

  InnerClassType dual_type(InnerClassType, const LieType& lt);

  latticetypes::LatticeMatrix involution(const lietype::LieType&,
					 const lietype::InnerClassType&);

  latticetypes::LatticeMatrix involution(const layout::Layout& lo);

  size_t rank(const LieType&);

  inline size_t rank(const SimpleLieType& slt) { return slt.second; }

  size_t semisimpleRank(const LieType&);

  inline size_t semisimpleRank(const SimpleLieType& slt)
  { return slt.first == 'T' ? 0UL : slt.second; }

  inline TypeLetter type(const SimpleLieType& slt) { return slt.first; }

}

}

#endif
