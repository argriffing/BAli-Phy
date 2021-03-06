/*
   Copyright (C) 2008 Benjamin Redelings

This file is part of BAli-Phy.

BAli-Phy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

BAli-Phy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with BAli-Phy; see the file COPYING.  If not see
<http://www.gnu.org/licenses/>.  */

///
/// \file slice-sampling.H
///
/// \brief This file defines classes and functions for uniform slice sampling.
///

#ifndef SLICE_SAMPLING_H
#define SLICE_SAMPLING_H

#include "parameters.H"

namespace slice_sampling {
  /// This function returns the value \a x that was passed in.
  double identity(double x);
}

/// A function interface object for use in slice sampling
struct slice_function 
{
  /// Compute the value of the function evaluated at x 
  virtual double operator()(double x)=0;

  /// Compute the value of the function evaluated at the current value
  virtual double operator()()=0;

  /// Return the current value of x
  virtual double current_value() const;

  /// is there a lower bound on the range?
  bool has_lower_bound;
  /// the value of the lower bound, if any
  double lower_bound;

  /// is there an upper bound on the range?
  bool has_upper_bound;
  /// the value of the upper bound, if any
  double upper_bound;

  /// check if a value is below the lower bound on the range
  bool below_lower_bound(double x) const { return (has_lower_bound and x<lower_bound); }
  /// check if a value is above the upper bound on the range
  bool above_upper_bound(double x) const { return (has_upper_bound and x>upper_bound); }
  /// check if a value is in the range or not
  bool in_range(double x) const  { return (not below_lower_bound(x) and not above_upper_bound(x));}

  /// set the lower bound on the range
  void set_lower_bound(double);

  /// set the upper bound on the range
  void set_upper_bound(double);

  slice_function();
};

double slice_sample(double x0, slice_function& g, double w, int m);

double slice_sample(slice_function& g, double w, int m);

std::pair<int,double> slice_sample_multi(double x0, std::vector<slice_function*>& g, double w, int m);

std::pair<int,double> slice_sample_multi(std::vector<double>& X0, std::vector<slice_function*>& g, double w, int m);

std::pair<int,double> slice_sample_multi(std::vector<slice_function*>& g, double w, int m);

/// A slice_function for slice-sampling a single parameter
struct parameter_slice_function:public slice_function
{
  int count;

  Parameters& P;

  int n;

  double operator()(double);

  double operator()();

  double current_value() const;

  // function to go from the stored value to the value on which the prior is.
  double (*transform)(double);
  // function to go from the value on which the prior is to the stored value.
  double (*inverse)(double);

  parameter_slice_function(Parameters&,int);
  parameter_slice_function(Parameters&,int,double(*)(double),double(*)(double));
};


/// A slice_function for slice-sampling a single branch length
struct branch_length_slice_function:public slice_function
{
  int count;

  Parameters& P;

  int b;

  double operator()(double);

  double operator()();

  double current_value() const;

  branch_length_slice_function(Parameters&,int);
};

/// A slice_function for slice-sampling two adjacent branch lengths which preserving their sum
struct slide_node_slice_function: public slice_function {
  int count;
  int b1;
  int b2;
  double total;

  Parameters& P;

  double operator()(double);
  double operator()();
  double current_value() const;
  slide_node_slice_function(Parameters&,int);
  slide_node_slice_function(Parameters&,int,int);
};

/// \brief A slice_function for changing only the branch length mean
///
/// This function is parameterized in terms of t, where
///   mu'[i] = mu[i] * exp(t)     for i=1..n branch means.
///   T'[i]  = T[i]  * exp(-t)    for i=1..B tranch lengths.
///
/// The slices are levels of \f$Pr(x)\times(\sum_i \mu_i)^{n-B}\f$ where
///   n = the number of means,
///   B = the number of branch lengths.
///
struct scale_means_only_slice_function: public slice_function
{
  /// The number of times the function is called.
  int count;

  /// The sum of the branch mean parameters for the initial point.
  double initial_sum_of_means;

  /// The object whose parameters we are sampling
  Parameters& P;

  /// return the log of (probability() * mu^(B-1)
  double operator()(double);

  double operator()();

  double current_value() const;

  scale_means_only_slice_function(Parameters&);
};

/// \brief A slice_function for resampling \f$x_n\f$ when \f$\sum_i x_i=C\f$
///
/// This function is parameterized in terms of t, where
///  x'[i] = x[i] * (C-t)/(C-x[n])   if (i!=n)
///  x'[n] = t
///
/// The slices are levels of \f$Pr(x)\times(C-x[n])^{N-1}$ where
///  N = the dimension of x.
///
struct constant_sum_slice_function:public slice_function
{
  /// The number of times the function is called.
  int count;

  /// Which indices form the dirichlet parameter that we are resampling?
  std::vector<int> indices;

  /// Which of the above indices is the one that we are resampling?
  int n;

  /// The object whose parameters we are sampling
  Parameters& P;

  /// return the probability * mu^(B-1)
  double operator()(double);

  double operator()();

  double current_value() const;

  constant_sum_slice_function(Parameters&,const std::vector<int>&,int);
};

double transform_epsilon(double);
double inverse_epsilon(double);

#endif
