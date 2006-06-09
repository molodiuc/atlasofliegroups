/*!
\file
  This is poset_fwd.h
*/
/*
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.4 

  See file main.cpp for full copyright notice
*/

#ifndef POSET_FWD_H  /* guard against multiple inclusions */
#define POSET_FWD_H

#include "set.h"

/******** forward type declarations *****************************************/

namespace atlas {

namespace poset {

  class Poset;
  typedef std::pair<set::SetElt,set::SetElt> Link;

  class SymmetricPoset;
}

}

#endif
