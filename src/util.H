/*
   Copyright (C) 2004-2009 Benjamin Redelings

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

/**
 * @file util.H
 *
 * @brief This file contains a number of utility routines.
 *
 */

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <algorithm>
#include "myexception.H"
#include <map>
#include <typeinfo>

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#define foreach(a,b) for(auto a=(b).begin();a != (b).end();a++)
#else
#define foreach(a,b) for(typeof((b).begin()) a=(b).begin();a != (b).end();a++)
#endif

// this should actually be templatized... <class charT,Alloc,Traits>
std::istream& getline_handle_dos(std::istream&,std::string&,char c='\n');

template <class T>
bool can_be_converted_to(const std::string& s, T& t) 
{ 
  std::istringstream i(s);
  i>>t;
  if (i and i.eof())
    return true;
  else
    return false;
}

template <class T>
T convertTo(const std::string& s) 
{ 
  T t;
  if (not can_be_converted_to<T>(s,t))
    throw myexception()<<"String '"<<s<<"' is not of type "<<typeid(T).name();
  return t;
}

template <class T>
std::vector<T> convertTo(const std::vector<std::string>& s) { 
  std::vector<T> v;
  for(int i=0;i<s.size();i++)
    v.push_back(convertTo<T>(s[i]));

  return v;
}

template <class T>
std::string convertToString(T t)
{
  std::ostringstream o;
  o.flags(o.flags() | std::ios::fixed);
  
  if (o << t)
    return o.str();
  // some sort of error handling goes here...
  return "conversion error";
} 

/// How many of v's bits are true?
int n_elements(const std::vector<bool>& v);

template<class T>
struct bad_mapping: public myexception
{
  T missing;

  int from;

  bad_mapping(const T& t,int f)
    : missing(t),from(f)
  { }

  bad_mapping(const T& t,int f,const std::string& s)
    : myexception(s),
      missing(t),
      from(f)
  { }

  ~bad_mapping() throw() {}
};

template <class T>
std::vector<int> compute_mapping(const std::vector<T>& v1,const std::vector<T>& v2) 
{
  std::vector<int> mapping(v1.size());
  for(int i=0;i<v1.size();i++) {
    int target = -1;
    for(int j=0;j<v2.size();j++) {
      if (v1[i] == v2[j]) {
	target = j;
	break;
      }
    }
    if (target == -1) throw bad_mapping<T>(v1[i],0,"Vector v2 does not contain all elements of v1!");

    mapping[i] = target;
  }

  if (v1.size() < v2.size())
    try {
      compute_mapping(v2,v1);
    }
    catch(bad_mapping<T>& b) {
      bad_mapping<T> b2(b.missing,1,"Vector v1 does not contain all elements of v2!");
      throw b2;
    }

  return mapping;
}

template <class T>
std::vector<T> apply_mapping(const std::vector<T>& v1,const std::vector<int>& mapping) {
  std::vector<T> v2(v1.size());
  for(int i=0;i<v2.size();i++)
    v2[mapping[i]] = v1[i];

  return v2;
}

template <typename T>
inline T reflect_less_than(T x,T max)
{
  T delta = x - max;
  if (delta > 0)
    x = max - delta;
  return x;
}

template <typename T>
inline T reflect_more_than(T x,T min)
{
  T delta = x - min;
  if (delta < 0)
    x = min - delta;
  return x;
}

///Reflect x at boundaries 0 and max to get a number in [0,max]
template <typename T>
inline T wrap(T x,T max) {
  if (max == 0)
    return 0;

  // flip around to position x axis
  if (x < 0)
    x = -x;

  // map to [0,2*max)
  int n = (int)(x/(2*max));
  x -= n*2*max;

  if (x > max)
    x = max*2 - x;

  assert(x >= 0 and x <= max);
  return x;
}

/// Reflect x at boundaries min and max to get a number in [min,max]
template <typename T>
inline T wrap(T x, T min, T max) {
  return wrap(x-min,max-min)+min;
}


template <typename T>
inline T minmax(T x,T min, T max) {
  if (x<min)
    return min;
  else if (x>max)
    return max;
  else
    return x;
}

/// Get index of t in v
template <typename T>
int find_index(const std::vector<T>& v,const T& t) 
{
  typename std::vector<T>::const_iterator loc = std::find(v.begin(),v.end(),t);
  if (loc == v.end())
    return -1;
  else
    return loc - v.begin();
}

/// Does v include t?
template <typename T>
bool includes(const std::vector<T>& v, const T& t) {
  return (std::find(v.begin(),v.end(),t) != v.end());
}

/// Does v1 include every member of v2?
template <typename T>
bool includes(const std::vector<T>& v1, const std::vector<T>& v2) {
  for(int i=0;i<v2.size();i++)
    if (not includes(v1,v2[i]))
      return false;
  return true;
}

/// Concatenate members of v, separating by char c
std::string join(const std::vector<std::string>& v,char c);

std::string join(const std::vector<std::string>& v,const std::string& s);

template<class T>
std::string join(const std::vector<T>& vT, char c)
{
  std::vector<std::string> v;
  for(int i=0;i<vT.size();i++)
    v.push_back(convertToString(vT[i]));
  return join(v,c);
}

template<class T>
std::string join(const std::vector<T>& vT, const std::string& s)
{
  std::vector<std::string> v;
  for(int i=0;i<vT.size();i++)
    v.push_back(convertToString(vT[i]));
  return join(v,s);
}

/// Split the line into pieces at c
std::vector<std::string> split(const std::string& line,char c);
std::vector<std::string> split(const std::string& line,const std::string& s);

/// Split the line into pieces at c and convert to T
template<class T>
std::vector<T> split(const std::string& s,char c) {
  std::vector<std::string> v = split(s,c);
  std::vector<T> vT;
  for(int i=0;i<v.size();i++)
    vT.push_back(convertTo<T>(v[i]));

  return vT;
}

template<class T>
std::vector<T> split(const std::string& s,const std::string& token) {
  std::vector<std::string> v = split(s,token);
  std::vector<T> vT;
  for(int i=0;i<v.size();i++)
    vT.push_back(convertTo<T>(v[i]));

  return vT;
}


/// Remove all occurrences of char c from line
std::string strip(const std::string& line,char c);
/// Remove all occurrences of each char in chars from line
std::string strip(const std::string& line,const std::string& chars);

/// invert the mapping
std::vector<int> invert(const std::vector<int>&);

/// compose the two mappings
std::vector<int> compose(const std::vector<int>&,const std::vector<int>&);

bool is_identity(const std::vector<int>&);

template <typename T>
std::vector<T> iota(T size)
{
  typedef typename std::vector<T>::size_type size_type;
  std::vector<T> v(size);
  for(size_type i=0;i<v.size();i++)
    v[i] = i;
  return v;
}

/// Return the min of v
template<class T>
T min(const std::vector<T>& v) {
  T t = v[0];
  for(int i=1;i<v.size();i++)
    t = std::min(t,v[i]);

  return t;
}

/// Return the arg-min of v
template<class T>
unsigned argmin(const std::vector<T>& v) {
  unsigned argmin = 0;
  for(unsigned i=1;i<v.size();i++)
    if (v[i] < v[argmin])
      argmin = i;

  return argmin;
}

/// Return the min of v
template<class T>
T max(const std::vector<T>& v) {
  T t = v[0];
  for(int i=1;i<v.size();i++)
    t = std::max(t,v[i]);

  return t;
}

/// Return the arg-max of v
template<class T>
unsigned argmax(const std::vector<T>& v) {
  unsigned argmax = 0;
  for(unsigned i=1;i<v.size();i++)
    if (v[i] > v[argmax])
      argmax = i;

  return argmax;
}

/// Order i~j by v[i]~v[j]
template <typename T>
struct sequence_order {
  const std::vector<T>& v;

  sequence_order(const std::vector<T>& v1):v(v1) {}

  bool operator()(int i,int j) const {
    return v[i]<v[j];
  }
};

/// If m contains name, convert m[name] to T and return.  Otherwise return value.
template <typename U, typename T>
bool set_if_undef(std::map<std::string,U,T>& m,const std::string& name, const U& value) {
  if (m.find(name) != m.end() )
    return false;
  else {
    m[name] = value;
    return true;
  }
}

/// If m contains name, convert m[name] to T and return.  Otherwise return value.
template <typename U, typename T>
const U& loadvalue(const std::map<std::string,U,T>& m,const std::string& name, const U& value) 
{
  typename std::map<std::string,U,T>::const_iterator here = m.find(name);
  if (here != m.end() )
    return here->second;
  else
    return value;
}

/// If m contains name, convert m[name] to T and return.  Otherwise return value.
template <typename U, typename T>
const U& loadvalue(std::map<std::string,U,T>& m,const std::string& name) 
{
  typename std::map<std::string,U,T>::const_iterator here = m.find(name);
  if (here != m.end() )
    return here->second;
  else
    throw myexception()<<"Parameter '"<<name<<"' not set.";
}


/// If m contains name, convert m[name] to T and return.  Otherwise return value.
template <typename U, typename T>
bool defined(std::map<std::string,U,T>& m,const std::string& name) 
{
  return (m.find(name) != m.end());
}

bool contains_char(const std::string& s,char c);

/// get the next word starting at position i, return true if not done.
bool get_word(std::string& word, int& i,const std::string& s,
	      const std::string& delimiters,const std::string& whitespace);

template<typename T>
T sum(const std::vector<T>& v)
{
  T total = 0;
  for(int i=0;i<v.size();i++)
    total += v[i];
  return total;
}

template<typename T,typename U>
void scale(std::vector<T>& v, const U& S)
{
  for(int i=0;i<v.size();i++)
    v[i] *= S;
}

template<typename T>
std::vector<T> select(const std::vector<T>& v1,bool pred(const T&))
{
  std::vector<T> v2;
  for(int i=0;i<v1.size();i++)
    if (pred(v1[i]))
      v2.push_back(v1[i]);
  return v2;
}

template<typename T>
std::vector<T> select(const std::vector<T>& v1,bool (T::*pred)() const)
{
  std::vector<T> v2;
  for(int i=0;i<v1.size();i++)
    if ((v1[i].*pred)())
      v2.push_back(v1[i]);
  return v2;
}

template <typename T>
std::vector<T> select(const std::vector<T>& v,const std::vector<int>& indices)
{
  std::vector<T> v2(indices.size());
  for(int i=0;i<v2.size();i++)
    v2[i] = v[indices[i]];
  return v2;
}

template<typename T>
unsigned count(const std::vector<T>& v1,bool pred(const T&))
{
  unsigned total=0;
  for(int i=0;i<v1.size();i++)
    if (pred(v1[i]))
      total++;
  return total;
}

template<typename T>
unsigned count(const std::vector<T>& v1,bool (T::*pred)() const)
{
  unsigned total=0;
  for(int i=0;i<v1.size();i++)
    if ((v1[i].*pred)())
      total++;
  return total;
}

template <typename T>
std::vector<T> read(const std::vector<T>& v,const std::vector<int>& indices)
{
  std::vector<T> values(indices.size());
  for(int i=0;i<indices.size();i++)
    values[i] = v[indices[i]];
  return values;
}


template <typename T>
std::vector<T> write(std::vector<T>& v,const std::vector<int>& indices,const std::vector<T>& values)
{
  assert(indices.size() == values.size());
  for(int i=0;i<indices.size();i++)
    v[indices[i]] = values[i];
  return values;
}

template <class T>
struct accumulator
{
  virtual void operator()(const T&)=0;
  virtual void finalize() {}
  virtual ~accumulator() {}
};

template <class T>
struct vector_accumulator: accumulator<T>,
			   public std::vector<T>
{
  void operator()(const T& t){push_back(t);}
};

template <typename T>
struct index_value
{
  int index;
  T value;
  T check_min(int index2, T value2) 
  {
    if (index == -1 or (value2 < value)) {
      index = index2;
      value = value2;
    }
    return value;
  }
  T check_max(int index2, T value2) 
  {
    if (index == -1 or (value2 > value)) {
      index = index2;
      value = value2;
    }
    return value;
  }
  index_value():index(-1),value() {}
  index_value(const T& t):index(-1),value(t) {}
};
  
void parse_simple_range(const std::string& range,int L,int& begin,int& end);
void parse_modulo_range(const std::string& range,int L,int& begin,int& end,int& step);
std::vector<int> parse_multi_range(const std::string& range,int L);

void scan_lines(std::istream& file,int skip,int subsample, int max, 
		accumulator<std::string>& op);

std::vector<std::string> load_lines(std::istream& file,int skip,int subsample, int max);

std::string get_basename(std::string filename);

std::string remove_extension(std::string filename);

std::vector<std::string> get_arguments(std::string& s,char begin, char end);

extern int log_verbose;
#endif
