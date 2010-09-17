/*!
\file
\brief Implementation for namespace lattice.

  This module defines some more general lattice
  functions, less simple than what is defined in latticetypes.cpp.
*/
/*
  This is lattice.cpp.

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#include "latticetypes.h"

#include "arithmetic.h"
#include "matreduc.h"

/*****************************************************************************

  This module defines some more general lattice functions, less simple than
  what is defined in latticetypes.cpp.

******************************************************************************/

namespace atlas {

  /*!
  \brief Functions for working with lattices.

  This namespace defines some more general lattice functions, less simple than
  what is defined in latticetypes.cpp. It includes change of basis functions,
  reduction modulo two, and calculating the orthogonal of a sublattice.
  */

/*****************************************************************************

        Chapter I -- Functions declared in lattice.h

******************************************************************************/

namespace lattice {

/*!
  In this template, we assume that |I|, and |O| are respectively random
  access input and output iterator types for type |Weight|, and that
  |[firstb,lastb[| holds a new $\Q$-basis for the lattice, in particular that
  |lastb-firstb| is equal to the size of the |Weight|s.

  As we iterate from |first| to |last|, we write the vectors in the
  new basis (this is supposed to be possible) and output the result to |O|.

  Doing the base change amounts to applying the inverse of |b|'s matrix.

  NOTE: we don't assume that |[firstb, lastb[| is necessarily a $\Z$-basis of
  the current lattice, only that it is a basis of a full rank sublattice
  containing the vectors in the input range; the new coordinates will then be
  integers. Users should be aware of the "full rank" condition; without it the
  specification still makes sense, but the implementation will fail.
*/
template<typename I, typename O>
  void baseChange(I first, I last, O out, I firstb, I lastb)
{
  latticetypes::LatticeCoeff d;
  latticetypes::LatticeMatrix q =
    latticetypes::LatticeMatrix(firstb,lastb,lastb-firstb,tags::IteratorTag())
    .inverse(d);

  while (first!=last)
  {
    *out = (q*(*first)/=d);
    ++out, ++first;
  }
}

/*!
  This (unsused) template function is like |baseChange|, but goes from weights
  expressed in terms of |[firstb, lastb[| to ones expressed in terms of the
  original basis. This is easier, as we don't have to invert the matrix!
*/
template<typename I, typename O>
  void inverseBaseChange(I first, I last, O out, I firstb, I lastb)
{
  latticetypes::LatticeMatrix q(firstb,lastb,lastb-firstb,tags::IteratorTag());

  while (first!= last)
  {
    *out = q*(*first);
    ++out, ++first;
  }
}


/*! \brief
  Returns a basis of the orthogonal of the sublattice generated by b in Z^r.

  Algorithm: diagonalise the matrix |M| with columns |b| using row operations
  |R| (forgetting column operations |C|) as for Smith form; then image of
  |R*M| is span of initial canonical basis vectors (as many as nonzero
  factors), and orthogonal sublattice is spanned by the remaining rows of |R|.

  Precondition: the (possibly dependent) vectors in |b| all have the size |r|.
*/
latticetypes::WeightList perp(const latticetypes::WeightList& b, size_t r)
{
  latticetypes::LatticeMatrix R,C;
  latticetypes::CoeffList factor =
    matreduc::diagonalise(latticetypes::LatticeMatrix(b,r),R,C);

  // drop any final factors 0
  size_t codim=factor.size();

  // collect final rows of |R|, those that annihilate the span of |b|
  latticetypes::WeightList result; result.reserve(r-codim);
  for (size_t i=codim; i<r; ++i)
    result.push_back(R.row(i));

  return result;
}

latticetypes::LatticeMatrix kernel(const latticetypes::LatticeMatrix& M)
{
  size_t n= M.numColumns(); // dimension of space to which |M| can be applied
  latticetypes::LatticeMatrix R,C;
  latticetypes::CoeffList factor = matreduc::diagonalise(M,R,C);

  size_t c=factor.size(); // codimension of kernel
  // now  $e_c, e_{c+1},..$ spans $\ker(R*M*C)$ and $\ker(M)=C.\ker(R*M*C)$

  return C.block(0,c,n,n); // whose row span is $C.\ker(R*M*C)$
}

latticetypes::LatticeMatrix eigen_lattice
  (latticetypes::LatticeMatrix M, latticetypes::LatticeCoeff lambda)
{
  size_t n=M.numRows();
  assert(n==M.numColumns());

  while (n-->0)
    M(n,n)-=lambda;

  return kernel(M);
}

latticetypes::LatticeMatrix row_saturate(const latticetypes::LatticeMatrix& M)
{
  size_t n= M.numColumns(); // dimension of space to which |M| can be applied
  latticetypes::CoeffList factor;
  latticetypes::LatticeMatrix b =
    matreduc::adapted_basis(M.transposed(),factor);

  size_t c=factor.size(); // rank of M (codimension of kernel)

  latticetypes::LatticeMatrix result(c,n); // initial rows of |b.treansposed()|
  for (size_t i=0; i<c; ++i)
    result.set_row(i,b.column(i));
  return result;
}

template
void baseChange
  (latticetypes::WeightList::iterator,
   latticetypes::WeightList::iterator,
   std::back_insert_iterator<latticetypes::WeightList>,
   latticetypes::WeightList::iterator,
   latticetypes::WeightList::iterator);


} // |namespace lattice|

} // |namespace atlas|
