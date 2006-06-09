/*!
\file
  This is tits_fwd.h
*/
/*
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.4 

  See file main.cpp for full copyright notice
*/

#ifndef TITS_FWD_H  /* guard against multiple inclusions */
#define TITS_FWD_H

#include <vector>

#include "constants.h"

/******** forward type declarations ******************************************/

namespace atlas {

namespace tits {

  class TitsElt;
  class TitsGroup;
  class TwistedTitsGroup;

  typedef std::vector<TitsElt> TitsEltList;

}

}

#endif
