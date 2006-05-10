#ifndef PROPOSALS_H
#define PROPOSALS_H

#include <valarray>

double dirichlet_fiddle(std::valarray<double>& p,double N);

double dirichlet_fiddle(std::valarray<double>& p,double N,const std::valarray<bool>& mask);

double dirichlet_fiddle_old(std::valarray<double>& p,double sigma);

double dirichlet_fiddle_old(std::valarray<double>& p,double sigma,const std::valarray<bool>& mask);

double scale_gaussian(double& x,double sigma);

#include "alignment.H"
#include "parameters.H"

class Proposal: public Cloneable {
public:
  Proposal* clone() const =0;
  virtual double operator()(alignment& A, Parameters& P) const=0;
};

typedef double (*generic_proposal)(alignment& A, Parameters& P);

class Generic_Proposal: public Proposal
{
  generic_proposal proposal;
  
public:
  Generic_Proposal* clone() const {return new Generic_Proposal(*this);}
  double operator()(alignment& A, Parameters& P) const {return proposal(A,P);}
  Generic_Proposal(generic_proposal gp):proposal(gp) {}
};

typedef double (*proposal_fn)(std::valarray<double>& x,const std::vector<double>& p);

class Proposal_Fn
{
  proposal_fn proposal;
public:
  virtual Proposal_Fn* clone() const {return new Proposal_Fn(*this);}

  virtual double operator()(std::valarray<double>& x,const std::vector<double>& p) const 
  {
    return proposal(x,p);
  }

  Proposal_Fn():proposal(NULL) {}
  Proposal_Fn(const proposal_fn& p):proposal(p) {}

  virtual ~Proposal_Fn() {}
};

class less_than: public Proposal_Fn
{
  double max;
  OwnedPointer<Proposal_Fn> proposal;
public:
  less_than* clone() const {return new less_than(*this);}
  double operator()(std::valarray<double>& x,const std::vector<double>& p) const;
  less_than(double m,const Proposal_Fn& P);
};

class more_than: public Proposal_Fn
{
  double min;
  OwnedPointer<Proposal_Fn> proposal;
public:
  more_than* clone() const {return new more_than(*this);}
  double operator()(std::valarray<double>& x,const std::vector<double>& p) const;
  more_than(double m,const Proposal_Fn& P);
};

class between: public Proposal_Fn
{
  double min;
  double max;
  OwnedPointer<Proposal_Fn> proposal;
public:
  between* clone() const {return new between(*this);}
  double operator()(std::valarray<double>& x,const std::vector<double>& p) const;
  between(double m1,double m2,const Proposal_Fn& P);
};

class log_scaled: public Proposal_Fn
{
  OwnedPointer<Proposal_Fn> proposal;
public:
  log_scaled* clone() const {return new log_scaled(*this);}
  double operator()(std::valarray<double>& x,const std::vector<double>& p) const;
  log_scaled(const Proposal_Fn& P);
};

class LOD_scaled: public Proposal_Fn
{
  OwnedPointer<Proposal_Fn> proposal;
public:
  LOD_scaled* clone() const {return new LOD_scaled(*this);}
  double operator()(std::valarray<double>& x,const std::vector<double>& p) const;
  LOD_scaled(const Proposal_Fn& P);
};

class Proposal2: public Proposal
{
  OwnedPointer<Proposal_Fn> proposal;
  std::vector<int> indices;
  std::vector<std::string> pnames;

public:
  Proposal2* clone() const {return new Proposal2(*this);}
  double operator()(alignment& A, Parameters& P) const;
  Proposal2(const Proposal_Fn& p,const std::string& s, const std::vector<string>& v,
	    const Parameters& P);
  Proposal2(const Proposal_Fn& p,const std::vector<std::string>& s, const std::vector<string>& v,
	    const Parameters& P);
};


double dirichlet_proposal(std::valarray<double>& x,const std::vector<double>& p);

double scale_gaussian2(std::valarray<double>& x,const std::vector<double>& p);

double shift_gaussian(std::valarray<double>& x,const std::vector<double>& p);

double shift_delta(std::valarray<double>& x, const std::vector<double>& p);

double shift_epsilon(std::valarray<double>& x, const std::vector<double>& p);


#endif