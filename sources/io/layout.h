/*
  This is layout.h

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#ifndef LAYOUT_H  /* guard against multiple inclusions */
#define LAYOUT_H

#include "layout_fwd.h"
#include "latticetypes_fwd.h"
#include "lietype.h"

#include "setutils.h"

namespace atlas {

/******** type definitions **************************************************/

namespace layout {

struct Layout {

  lietype::LieType d_type;
  lietype::InnerClassType d_inner;
  latticetypes::WeightList d_basis;
  setutils::Permutation d_perm;

// constructors and destructors
  Layout() {}

  /* In the old atlas interface, the Lie type and lattice basis are first
     provided, and the inner class type is later added via user interaction */
  Layout(const lietype::LieType& lt, const latticetypes::WeightList& b)
  :d_type(lt),d_inner(),d_basis(b),d_perm(lietype::rank(lt),1) {}

  /* In the new atlas interface, the Lie type and inner class type are
     computed from the involution, and the lattice basis is unused */
  Layout(const lietype::LieType& lt, const lietype::InnerClassType ict)
  :d_type(lt),d_inner(ict), d_basis(),d_perm(lietype::rank(lt),1) {}

  ~Layout() {}

// manipulators
  void swap (Layout& source) {
    d_type.swap(source.d_type);
    d_inner.swap(source.d_inner);
    d_basis.swap(source.d_basis);
    d_perm.swap(source.d_perm);
  }

};

}

}

#endif
