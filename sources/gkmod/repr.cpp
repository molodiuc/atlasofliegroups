/*
  This is repr.cpp

  Copyright (C) 2009 Marc van Leeuwen
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#include "repr.h"

#include "arithmetic.h"
#include "tits.h"

#include "kgb.h"	// various methods
#include "blocks.h"	// |dual_involution|
#include "standardrepk.h"// |KhatContext| methods

namespace atlas {
  namespace repr {

bool StandardRepr::operator== (const StandardRepr& z) const
{ return x_part==z.x_part and y_bits==z.y_bits
  and infinitesimal_char==z.infinitesimal_char;
}

Rep_context::Rep_context(RealReductiveGroup &G_R)
  : G(G_R), KGB_set(G_R.kgb())
{}

const TwistedInvolution Rep_context::twistedInvolution(size_t cn) const
{ return complexGroup().twistedInvolution(cn); }

StandardRepr
  Rep_context::sr
    (const standardrepk::StandardRepK& srk,
     const standardrepk::KhatContext& khc,
     const RatWeight& nu) const
{
  TitsElt a = khc.titsElt(srk); // was reduced during construction |srk|
  KGBElt x= khc.kgb().lookup(a,titsGroup());
  InvolutionNbr i_x = kgb().inv_nr(x);
  const InvolutionTable& i_tab = complexGroup().involution_table();
  const WeightInvolution& theta = i_tab.matrix(i_x);

  Weight lambda2 = khc.lift(srk); // doubled coordinates
  RatWeight lambda(lambda2,2);
  RatWeight diff = lambda - nu;
  RatWeight theta_diff(theta*diff.numerator(),
		       diff.denominator()); // theta(lambda-nu)
  Weight lambda_rho = (lambda2-khc.rootDatum().twoRho())/=2;
  return StandardRepr(x,i_tab.pack(i_x,lambda_rho),
		      ((lambda+nu+theta_diff)/=2).normalize());
}

StandardRepr Rep_context::sr
  (KGBElt x, const Weight lambda_rho, const RatWeight& nu) const
{
  InvolutionNbr i_x = kgb().inv_nr(x);
  const InvolutionTable& i_tab = complexGroup().involution_table();
  const WeightInvolution& theta = i_tab.matrix(i_x);
  RatWeight lambda(lambda_rho*2+rootDatum().twoRho(),2);
  RatWeight diff = lambda - nu;
  RatWeight theta_diff(theta*diff.numerator(),
		       diff.denominator()); // theta(lambda-nu)
  return StandardRepr(x,i_tab.pack(i_x,lambda_rho),
		      ((lambda+nu+theta_diff)/=2).normalize());
}

Weight Rep_context::lambda_rho(const StandardRepr& z) const
{
  InvolutionNbr i_x = kgb().inv_nr(z.x());
  const InvolutionTable& i_tab = complexGroup().involution_table();
  const WeightInvolution& theta = i_tab.matrix(i_x);

  RatWeight gamma_rho = z.gamma() - RatWeight(rootDatum().twoRho(),2);
  Weight im_part2 = gamma_rho.numerator()+theta*gamma_rho.numerator();
  im_part2 /= gamma_rho.denominator(); // exact: $(1+\theta)(\lambda-\rho)$
  return (im_part2 + i_tab.unpack(i_x,z.y()))/=2; // division exact again
}

// return $\lambda \in \rho+X^*$ as half-integer rational vector
RatWeight Rep_context::lambda(const StandardRepr& z) const
{
  Weight num = lambda_rho(z) * 2 + rootDatum().twoRho();
  return RatWeight(num,2).normalize();
}

RatWeight Rep_context::nu(const StandardRepr& z) const
{
  InvolutionNbr inv = kgb().inv_nr(z.x());
  const WeightInvolution& theta = complexGroup().involution_table().matrix(inv);
  Weight num = z.gamma().numerator()-theta*z.gamma().numerator();
  return RatWeight(num,2*z.gamma().denominator()).normalize();
}

// convert |lambda| and |gamma| into $y$ value
// Formula: $\exp(i\pi(\gamma-\lambda)) \sigma_{tw} \delta_1$
GlobalTitsElement Rep_context::y(const StandardRepr& z) const
{
  TorusElement t = y_values::exp_pi(z.gamma()-lambda(z));
  const TwistedWeylGroup& W = KGB_set.twistedWeylGroup();
  const TwistedWeylGroup dual_W (W,tags::DualTag()); // VERY EXPENSIVE!
  TwistedInvolution tw =
    blocks::dual_involution(KGB_set.involution(z.x()),W,dual_W);
  return GlobalTitsElement(t,tw);
}

// |z| standard means (weakly) dominant on the (simple-)imaginary roots
bool Rep_context::is_standard(const StandardRepr& z, RootNbr& witness) const
{
  const RootDatum& rd = rootDatum();
  InvolutionNbr i_x = kgb().inv_nr(z.x());
  const InvolutionTable& i_tab = complexGroup().involution_table();
  for (unsigned i=0; i<i_tab.imaginary_rank(i_x); ++i)
  {
    RootNbr alpha = i_tab.imaginary_basis(i_x,i);
    Weight lr = lambda_rho(z);
    int v = lr.scalarProduct(rd.coroot(alpha))+rd.colevel(alpha);
    if (v<0)
      return witness=alpha,false;
  }
  return true;
}

// |z| zero means that no singular imaginary roots are compact
bool Rep_context::is_zero(const StandardRepr& z, RootNbr& witness) const
{
  const RootDatum& rd = rootDatum();
  InvolutionNbr i_x = kgb().inv_nr(z.x());
  const InvolutionTable& i_tab = complexGroup().involution_table();
  for (unsigned i=0; i<i_tab.imaginary_rank(i_x); ++i)
  {
    RootNbr alpha = i_tab.imaginary_basis(i_x,i);
    int v = lambda_rho(z).scalarProduct(rd.coroot(alpha))+rd.colevel(alpha);
    bool compact =
      kgb::status(kgb(),z.x(),rd,alpha)==gradings::Status::ImaginaryCompact;
    if (v==0 and compact)
      return witness=alpha,true;
  }
  return false;
}


// |z| final means that no singular real roots satisfy the parity condition
bool Rep_context::is_final(const StandardRepr& z, RootNbr& witness) const
{
  const RootDatum& rd = rootDatum();
  InvolutionNbr i_x = kgb().inv_nr(z.x());
  const InvolutionTable& i_tab = complexGroup().involution_table();
  RootNbrSet pos_real = i_tab.real_roots(i_x) & rd.posRootSet();
  Weight lambda2_shift=lambda_rho(z)*2 + rd.twoRho()-rd.twoRho(pos_real);

  for (RootNbrSet::iterator it=pos_real.begin(); it(); ++it)
  {
    const Weight& av = rootDatum().coroot(*it);
    if (av.dot(z.gamma().numerator())==0 and
	av.dot(lambda2_shift)%4 !=0) // singular yet odd on shifted lambda
      return witness=*it,false;
  }
  return true;
}

bool Rep_context::is_oriented(const StandardRepr& z, RootNbr alpha) const
{
  const RootDatum& rd = rootDatum();
  InvolutionNbr inv = kgb().inv_nr(z.x());
  RootNbrSet real = complexGroup().involution_table().real_roots(inv);

  assert(real.isMember(alpha)); // only real roots should be tested

  const Weight& av = rootDatum().coroot(alpha);
  int numer = av.dot(z.gamma().numerator());
  int denom = z.gamma().denominator();
  assert(numer%denom!=0); // and the real root alpha should be non-integral

  Weight lambda2_shift=lambda_rho(z)*2 + rd.twoRho()-rd.twoRho(real);
  int eps = av.dot(lambda2_shift)%4==0 ? 0 : denom;

  return arithmetic::remainder(numer+eps,2*denom)< (unsigned)denom;
}

unsigned int Rep_context::orientation_number(const StandardRepr& z) const
{
  const RootDatum& rd = rootDatum();
  const InvolutionTable& i_tab = complexGroup().involution_table();
  InvolutionNbr inv = kgb().inv_nr(z.x());
  RootNbrSet real = i_tab.real_roots(inv);
  const Permutation& root_inv = i_tab.root_involution(inv);
  const Weight& numer = z.gamma().numerator();
  int denom = z.gamma().denominator();
  Weight lambda2_shift=lambda_rho(z)*2 + rd.twoRho()-rd.twoRho(real);

  unsigned count = 0;

  for (unsigned i=0; i<rd.numPosRoots(); ++i)
  {
    RootNbr alpha = rd.numPosRoots()+i;
    const Weight& av = rootDatum().coroot(alpha);
    int num = av.dot(numer);
    if (num%denom!=0) // skip integral roots
    { if (real.isMember(alpha))
      {
	int eps = av.dot(lambda2_shift)%4==0 ? 0 : denom;
	if ((num>0) == // either positive for gamma and oriente, or neither
	    (arithmetic::remainder(num+eps,2*denom)< (unsigned)denom))
	  ++count;
      }
      else // complex root
      {
	assert(i_tab.complex_roots(inv).isMember(alpha));
	RootNbr beta = root_inv[alpha];
	if (i<rd.rt_abs(beta) // consider only first conjugate "pair"
	    and (num>0)!=(rootDatum().coroot(beta).dot(numer)>0))
	  ++count;
      }
    }
  }
  return count;
}

StandardRepr& Rep_context::make_dominant(StandardRepr& z) const
{
  const RootDatum& rd = rootDatum();
  const InvolutionTable& i_tab = complexGroup().involution_table();
  Weight lr = lambda_rho(z);
  KGBElt& x = z.x_part;
  Weight& numer = z.infinitesimal_char.numerator();
  InvolutionNbr i_x = kgb().inv_nr(x);

  { weyl::Generator s;
    do
    {
      for (s=0; s<rd.semisimpleRank(); ++s)
      {
	int v=rd.simpleCoroot(s).dot(numer);
        if (v<0 or (v==0 and kgb().isComplexDescent(s,x)))
        {
	  RootNbr alpha = rd.simpleRootNbr(s);
	  if (i_tab.imaginary_roots(i_x).isMember(alpha))
	    throw std::runtime_error("Non standard parameter in make_dominant");
          rd.simpleReflect(numer,s);
          rd.simpleReflect(lr,s);
	  if (not i_tab.real_roots(i_x).isMember(alpha)) // if |alpha| is real
	    lr -= rd.simpleRoot(s); // then $\rho_r$ cancels $\rho$
          x = kgb().cross(s,x);
	  i_x = kgb().inv_nr(x); // keep up with changing involution
          break;
        }
      }
    }
    while (s<rd.semisimpleRank()); // wait until inner loop runs to completion
  }
  z.y_bits=i_tab.pack(i_x,lr);
  return z;
}

Rep_context::compare Rep_context::repr_less() const
{ return compare(rootDatum().dual_twoRho()); }

bool Rep_context::compare::operator()
  (const StandardRepr& r,const StandardRepr& s) const
{
  int rgd=r.gamma().denominator(), sgd=s.gamma().denominator();
  int lr = sgd*level_vec.dot(r.gamma().numerator());
  int ls = rgd*level_vec.dot(s.gamma().numerator());
  if (lr!=ls)
    return lr<ls;
  for (size_t i=0; i<level_vec.size(); ++i)
    if (sgd*r.gamma().numerator()[i]!=rgd*s.gamma().numerator()[i])
      return sgd*r.gamma().numerator()[i]<rgd*s.gamma().numerator()[i];
  if (r.x()!=s.x())
    return r.x()<s.x();
  return r.y()<s.y();
}

SR_poly Rep_context::expand_final(StandardRepr z) const // by value
{
  const RootDatum& rd = rootDatum();
  const InvolutionTable& i_tab = complexGroup().involution_table();

  make_dominant(z); // this simplifies matters a lot; |z| is unchanged hereafter

  InvolutionNbr i_x = kgb().inv_nr(z.x());
  const RatWeight& gamma=z.gamma();

  RankFlags singular_real_parity;
  for (weyl::Generator s=0; s<rd.semisimpleRank(); ++s)
    if (gamma.numerator().dot(rd.simpleCoroot(s))==0)
    { if (i_tab.is_real_simple(i_x,s))
	singular_real_parity.set // record whether |s| is a real parity root
	  // |unpack| gives $(1-\theta)(\lambda-\rho)$
	  // real simple coroot odd on $\lambda-\rho$ means it is parity
	  (s,rd.simpleCoroot(s).dot(i_tab.unpack(i_x,z.y()))%4!=0);
      else if (i_tab.is_imaginary_simple(i_x,s))
      {
	if (kgb().status(s,z.x())==gradings::Status::ImaginaryCompact)
	  return SR_poly(repr_less());; // return a zero result
      }
      else
	assert(not kgb().isComplexDescent(s,z.x())); // because |make_dominant|
    }
  // having made dominant, any non-final is witnessed on a (real) simple root
  if (singular_real_parity.any())
  {
    const weyl::Generator s= singular_real_parity.firstBit();
    const KGBEltPair p = kgb().inverseCayley(s,z.x());
    Weight lr = lambda_rho(z); // is passed unchanged to inverse Cayleys
    lr -= // correct so that $\<\lambda,\alpha^\vee>=0$, like $\gamma$
      rd.simpleRoot(s)*((rd.simpleCoroot(s).dot(lr)+1)/2);
    assert(rd.simpleCoroot(s).dot(lr)==-1); // because $\<\rho,\alpha^\vee>=1$
    SR_poly result = expand_final(sr(p.first,lr,gamma));
    if (p.second!=UndefKGB)
      result += expand_final(sr(p.second,lr,gamma));
    return result;
  }
  else return SR_poly(z,repr_less());
}

  } // |namespace repr|
} // |namespace atlas|
