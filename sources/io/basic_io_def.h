/*
  This is basic_io_def.h
  
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups 

  See file main.cpp for full copyright notice
*/

#include <iostream>

/*****************************************************************************

        Chapter I -- Template functions defined in basic_io.h

******************************************************************************/

namespace atlas {

namespace basic_io {

template<size_t dim>
  std::ostream& operator<< (std::ostream& strm, 
			    const bitvector::BitVector<dim>& v)

/*
  Prints the bits of v on strm left-to-right
*/

{
  for (size_t j = 0; j < v.size(); ++j)
    if (v.test(j))
      strm << "1";
    else
      strm << "0";

  return strm;
}

template<size_t d>
  std::ostream& operator<< (std::ostream& strm, const bitset::BitSet<d>& b)

/*
  Prints the bits of b on strm left-to-right
*/

{
  for (size_t j = 0; j < d; ++j)
    if (b.test(j))
      strm << "1";
    else
      strm << "0";

  return strm;
}

template<typename I>
std::ostream& seqPrint(std::ostream& strm, const I& first, const I& last, 
		       const char* sep, const char* pre, const char* post)

/*
  This is a function for sequential output. It is assumed that I is an
  iterator type, pointing to a type for which the << operator is defined.
  Then we output the elements of the range [first,last[ as a list,
  with prefix pre, postfix post, and separator sep.
*/

{
  strm << pre;
  bool firstElt = true;

  for (I i = first; i != last; ++i) {
    if (firstElt)
      firstElt = false;
    else
      strm << sep;
    strm << *i;
  }

  strm << post;

  return strm;
}

}

}
