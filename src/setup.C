/*
   Copyright (C) 2004-2010 Benjamin Redelings

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
 * @file setup.C
 *
 * @brief This file contains routines for parsing the command line and
 *        loading information from files in order to start the MCMC
 *        analysis.
 */

#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/shared_ptr.hpp>

#include "setup.H"
#include "smodel.H"
#include "imodel.H"
#include "util.H"
#include "rates.H"
#include "alphabet.H"
#include "alignment-util.H"
#include "tree-util.H"
#include "substitution-index.H"

using std::ifstream;
using std::string;
using std::vector;
using std::valarray;
using std::cout;
using std::cerr;
using std::endl;

namespace fs = boost::filesystem;

using namespace boost::program_options;
using boost::shared_ptr;

/// Reorder internal sequences of \a A to correspond to standardized node names for \a T
alignment standardize(const alignment& A, const SequenceTree& T) 
{
  SequenceTree T2 = T;

  // if we don't have any internal node sequences, then we are already standardized
  if (A.n_sequences() == T.n_leaves())
    return A;

  // standardize NON-LEAF node and branch names in T
  vector<int> mapping = T2.standardize();
  vector<int> new_order = invert(mapping);

  return reorder_sequences(A,new_order);
}

/// Count the number of times the letter with index \a l occurs in \a A.
int letter_count(const alignment& A,int l) 
{
  // Count the occurrence of the different letters
  int count=0;
  for(int i=0;i<A.length();i++)
    for(int j=0;j<A.n_sequences();j++)
      if (A(i,j) == l)
	count++;

  return count;
}

/// Compute the number of times each letter of the alphabet occurs in \a A.
valarray<double> letter_counts(const alignment& A) 
{
  const alphabet& a = A.get_alphabet();

  // Count the occurrence of the different letters
  valarray<double> counts(0.0, a.size());
  for(int i=0;i<A.length();i++) {
    for(int j=0;j<A.n_sequences();j++) {
      if (a.is_letter(A(i,j)))
	counts[A(i,j)]++;
    }
  }

  return counts;
}


/// \brief Estimate the empirical frequencies of different letters from the alignment, with pseudocounts
///
/// \param args The command line parameters.
/// \param A The alignment.
///
valarray<double> empirical_frequencies(const variables_map& args,const alignment& A) 
{
  const alphabet& a = A.get_alphabet();

  // Count the occurrence of the different letters
  valarray<double> counts = letter_counts(A);

  valarray<double> frequencies(a.size());

  // empirical frequencies
  if (not args.count("frequencies"))
    frequencies = A.get_alphabet().get_frequencies_from_counts(counts,A.n_sequences()/2);

  // uniform frequencies
  else if (args["frequencies"].as<string>() == "uniform")
    frequencies = 1.0/a.size();

  // triplet frequencies <- nucleotide frequencies
  else if (args["frequencies"].as<string>() == "nucleotides") {
    const Triplets* T = dynamic_cast<const Triplets*>(&a);

    if (not T) throw myexception()<<"You can only specify nucleotide frequencies on Triplet or Codon alphabets.";
    valarray<double> N_counts = get_nucleotide_counts_from_codon_counts(*T,counts);
    valarray<double> fN = T->getNucleotides().get_frequencies_from_counts(N_counts,A.n_sequences()/2);

    frequencies = get_codon_frequencies_from_independent_nucleotide_frequencies(*T,fN);
  }

  // specified frequencies
  else {
    vector<double> f = split<double>(args["frequencies"].as<string>(),',');

    if (f.size() != a.size())
      throw myexception()<<"You specified "<<f.size()<<" frequencies, but there are "
			 <<a.size()<<" letters of the alphabet!";

    for(int i=0;i<f.size();i++)
      frequencies[i] = f[i];
  }

  return frequencies;
}

/// \brief Estimate the empirical frequencies of different letters from alignments, with pseudocounts
///
/// \param args The command line parameters.
/// \param alignments The alignments.
///
valarray<double> empirical_frequencies(const variables_map& args,const vector<alignment>& alignments) 
{
  int total=0;
  for(int i=0;i<alignments.size();i++)
    total += alignments[i].length();

  alignment A(alignments[0]);
  A.changelength(total);

  int L=0;
  for(int i=0;i<alignments.size();i++) 
  {
    for(int c=0;c<alignments[i].length();c++)
      for(int s=0;s<alignments[i].n_sequences();s++)
	A(c+L,s) = alignments[i](c,s);
    L += alignments[i].length();
  }

  return empirical_frequencies(args,A);
}

/// \brief Re-index the leaves of tree \a T so that the labels have the same ordering as in \a names.
///
/// \param T The leaf-labelled tree.
/// \param names The ordered leaf labels.
///
void remap_T_indices(SequenceTree& T,const vector<string>& names)
{
  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  try {
    vector<int> mapping = compute_mapping(T.get_sequences(),names);

    T.standardize(mapping);
  }
  catch(const bad_mapping<string>& b)
  {
    bad_mapping<string> b2(b.missing,b.from);
    if (b.from == 0)
      b2<<"Couldn't find leaf sequence \""<<b2.missing<<"\" in names.";
    else
      b2<<"Sequence '"<<b2.missing<<"' not found in the tree.";
    throw b2;
  }
}

/// \brief Re-index the leaves of tree \a T so that the labels have the same ordering as in \a A.
///
/// \param T The leaf-labelled tree.
/// \param A A multiple sequence alignment.
///
void remap_T_indices(SequenceTree& T,const alignment& A)
{
  if (A.n_sequences() < T.n_leaves())
    throw myexception()<<"Tree has "<<T.n_leaves()<<" leaves, but alignment has only "<<A.n_sequences()<<" sequences.";

  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  try {
    vector<string> names = sequence_names(A,T.n_leaves());  

    remap_T_indices(T,names);
  }
  catch(const bad_mapping<string>& b)
  {
    bad_mapping<string> b2(b.missing,b.from);
    if (b.from == 0)
      b2<<"Couldn't find leaf sequence \""<<b2.missing<<"\" in alignment.";
    else
      b2<<"Alignment sequence '"<<b2.missing<<"' not found in the tree.";
    throw b2;
  }
}

/// \brief Re-index the leaves of tree \a T1 so that the labels have the same ordering as in \a T2.
///
/// \param T1 The leaf-labelled tree to re-index.
/// \param T2 The leaf-labelled tree to match.
///
void remap_T_indices(SequenceTree& T1,const SequenceTree& T2)
{
  if (T1.n_leaves() != T2.n_leaves())
    throw myexception()<<"Trees do not correspond: different numbers of leaves.";

  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  try {
    remap_T_indices(T1,T2.get_sequences());
  }
  catch(const bad_mapping<string>& b)
  {
    bad_mapping<string> b2(b.missing,b.from);
    if (b.from == 0)
      b2<<"Couldn't find leaf sequence \""<<b2.missing<<"\" in second tree.";
    else
      b2<<"Couldn't find leaf sequence \""<<b2.missing<<"\" in first tree.";
    throw b2;
  }
}

/// \brief  Remap the leaf indices of tree \a T to match the alignment \a A: check the result
///
/// \param A The alignment.
/// \param T The tree.
/// \param internal_sequences Should the resulting alignment have sequences for internal nodes on the tree?
///
void link(alignment& A,SequenceTree& T,bool internal_sequences) 
{
  check_names_unique(A);

  // Later, might we WANT sub-branches???
  if (has_sub_branches(T))
    remove_sub_branches(T);

  if (internal_sequences and not is_Cayley(T)) {
    assert(has_polytomy(T));
    throw myexception()<<"Cannot link a multifurcating tree to an alignment with internal sequences.";
  }

  //------ IF sequences < leaf nodes THEN complain ---------//
  if (A.n_sequences() < T.n_leaves())
    throw myexception()<<"Tree has "<<T.n_leaves()<<" leaves but Alignment only has "
		       <<A.n_sequences()<<" sequences.";

  //----- IF sequences = leaf nodes THEN maybe add internal sequences.
  else if (A.n_sequences() == T.n_leaves()) {
    if (internal_sequences)
      A = add_internal(A,T);
  }
  //----- IF sequences > leaf nodes THEN maybe complain -------//
  else {
    if (not internal_sequences) {
      alignment A2 = chop_internal(A);
      if (A2.n_sequences() == T.n_leaves()) {
	A = A2;
      }
      else
	throw myexception()<<"More alignment sequences than leaf nodes!";
    } 
    else if (A.n_sequences() > T.n_nodes())
      throw myexception()<<"More alignment sequences than tree nodes!";
    else if (A.n_sequences() < T.n_nodes())
      throw myexception()<<"Fewer alignment sequences than tree nodes!";
  }
  
  //---------- double-check that we have the right number of sequences ---------//
  if (internal_sequences)
    assert(A.n_sequences() == T.n_nodes());
  else
    assert(A.n_sequences() == T.n_leaves());

  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  remap_T_indices(T,A);

  if (internal_sequences)
    connect_leaf_characters(A,T);

  //---- Check to see that internal nodes satisfy constraints ----//
  check_alignment(A,T,internal_sequences);
}

/// \brief  Remap the leaf indices of tree \a T to match the alignment \a A: check the result
///
/// \param A The alignment.
/// \param T The tree.
/// \param internal_sequences Should the resulting alignment have sequences for internal nodes on the tree?
///
void link(alignment& A,RootedSequenceTree& T,bool internal_sequences) 
{
  check_names_unique(A);

  // Later, might we WANT sub-branches???
  if (has_sub_branches(T))
    remove_sub_branches(T);

  if (internal_sequences and not is_Cayley(T)) {
    assert(has_polytomy(T));
    throw myexception()<<"Cannot link a multifurcating tree to an alignment with internal sequences.";
  }

  //------ IF sequences < leaf nodes THEN complain ---------//
  if (A.n_sequences() < T.n_leaves())
    throw myexception()<<"Tree has "<<T.n_leaves()<<" leaves but Alignment only has "
		       <<A.n_sequences()<<" sequences.";

  //----- IF sequences = leaf nodes THEN maybe add internal sequences.
  else if (A.n_sequences() == T.n_leaves()) {
    if (internal_sequences)
      A = add_internal(A,T);
  }
  //----- IF sequences > leaf nodes THEN maybe complain -------//
  else {
    if (not internal_sequences)
      throw myexception()<<"More alignment sequences than leaf nodes!";

    if (A.n_sequences() > T.n_nodes())
      throw myexception()<<"More alignment sequences than tree nodes!";
    else if (A.n_sequences() < T.n_nodes())
      throw myexception()<<"Fewer alignment sequences than tree nodes!";
  }
  
  //---------- double-check that we have the right number of sequences ---------//
  if (internal_sequences)
    assert(A.n_sequences() == T.n_nodes());
  else
    assert(A.n_sequences() == T.n_leaves());


  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  remap_T_indices(T,A);

  if (internal_sequences)
    connect_leaf_characters(A,T);

  //---- Check to see that internal nodes satisfy constraints ----//
  check_alignment(A,T,internal_sequences);
}

/// \brief Reorder leaf indices of T and sequences indices of alignments to match alignments[0]; check the result.
///
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
///
void link(vector<alignment>& alignments, SequenceTree& T, const vector<bool>& internal_sequences)
{
  for(int i=1;i<alignments.size();i++)
  {
    if (alignments[i].n_sequences() != alignments[0].n_sequences())
      throw myexception()<<"Alignment #"<<i+1<<" has "<<alignments[i].n_sequences()<<" sequences, but the previous alignments have "<<alignments[0].n_sequences()<<" sequences!";

    vector<int> mapping = compute_mapping(sequence_names(alignments[i]),sequence_names(alignments[0]));
    vector<int> new_order = invert(mapping);

    alignments[i] = reorder_sequences(alignments[i],new_order);
  }

  for(int i=0;i<alignments.size();i++) 
    link(alignments[i],T,internal_sequences[i]);
}

/// \brief Reorder leaf indices of T and sequences indices of alignments to match alignments[0]; check the result.
///
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
///
void link(vector<alignment>& alignments, RootedSequenceTree& T, const vector<bool>& internal_sequences)
{
  for(int i=1;i<alignments.size();i++)
  {
    if (alignments[i].n_sequences() != alignments[0].n_sequences())
      throw myexception()<<"Alignment #"<<i+1<<" has "<<alignments[i].n_sequences()<<" sequences, but the previous alignments have "<<alignments[0].n_sequences()<<" sequences!";

    vector<int> mapping = compute_mapping(sequence_names(alignments[i]),sequence_names(alignments[0]));
    vector<int> new_order = invert(mapping);

    alignments[i] = reorder_sequences(alignments[i],new_order);
  }

  for(int i=0;i<alignments.size();i++) 
    link(alignments[i],T,internal_sequences[i]);
}

/// Load an collections of alignments from command line args "--align filename1 --align filename2 ... "
vector<alignment> load_As(const variables_map& args)
{
  vector<string> filenames = args["align"].as<vector<string> >();

  shared_items<string> alphabet_names = get_mapping(args, "alphabet", filenames.size());

  vector<alignment> alignments;

  for(int i=0;i<filenames.size();i++) {
    const string alphabet_name = alphabet_names[i];
    if (alphabet_name.size())
      alignments.push_back( load_alignment(filenames[i], load_alphabets(alphabet_name) ) );
    else
      alignments.push_back( load_alignment(filenames[i], load_alphabets() ) );
  }

  return alignments;
}

/// \brief Load a tree and a collection of alignments based on command line parameters.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_As_and_T(const variables_map& args,vector<alignment>& alignments,SequenceTree& T,bool internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  vector<bool> i(filenames.size(),internal_sequences);

  load_As_and_T(args,alignments,T,i);
}

/// \brief Load a tree and a collection of alignments based on command line parameters.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_As_and_T(const variables_map& args,vector<alignment>& alignments,SequenceTree& T,const vector<bool>& internal_sequences)
{
  alignments = load_As(args);

  T = load_T(args);

  link(alignments,T,internal_sequences);

  for(int i=0;i<alignments.size();i++) 
  {
    
    //---------------- Randomize alignment? -----------------//
    if (args.count("randomize-alignment"))
      alignments[i] = randomize(alignments[i],T.n_leaves());
  
    //------------------ Analyze 'internal'------------------//
    if ((args.count("internal") and args["internal"].as<string>() == "+")
	or args.count("randomize-alignment"))
      for(int column=0;column< alignments[i].length();column++) {
	for(int j=T.n_leaves();j<alignments[i].n_sequences();j++) 
	  alignments[i](column,j) = alphabet::not_gap;
      }

    //---- Check that internal sequence satisfy constraints ----//
    check_alignment(alignments[i],T,internal_sequences[i]);
  }
}

/// \brief Load a tree and a collection of alignments based on command line parameters.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_As_and_T(const variables_map& args,vector<alignment>& alignments,RootedSequenceTree& T,bool internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  vector<bool> i(filenames.size(),internal_sequences);

  load_As_and_T(args,alignments,T,i);
}

/// \brief Load a tree and a collection of alignments based on command line parameters.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_As_and_T(const variables_map& args,vector<alignment>& alignments,RootedSequenceTree& T,const vector<bool>& internal_sequences)
{
  alignments = load_As(args);

  T = load_T(args);

  link(alignments,T,internal_sequences);

  for(int i=0;i<alignments.size();i++) 
  {
    
    //---------------- Randomize alignment? -----------------//
    if (args.count("randomize-alignment"))
      alignments[i] = randomize(alignments[i],T.n_leaves());
  
    //------------------ Analyze 'internal'------------------//
    if ((args.count("internal") and args["internal"].as<string>() == "+")
	or args.count("randomize-alignment"))
      for(int column=0;column< alignments[i].length();column++) {
	for(int j=T.n_leaves();j<alignments[i].n_sequences();j++) 
	  alignments[i](column,j) = alphabet::not_gap;
      }

    //---- Check that internal sequence satisfy constraints ----//
    check_alignment(alignments[i],T,internal_sequences[i]);
  }
}


/// \brief Load a collection of alignments based on command line parameters and generate a random tree.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_As_and_random_T(const variables_map& args,vector<alignment>& alignments,SequenceTree& T,bool internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  vector<bool> i(filenames.size(),internal_sequences);

  load_As_and_random_T(args,alignments,T,i);
}


/// \brief Load a collection of alignments based on command line parameters and generate a random tree.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_As_and_random_T(const variables_map& args,vector<alignment>& alignments,SequenceTree& T,const vector<bool>& internal_sequences)
{
  alignments = load_As(args);

  //------------- Load random tree ------------------------//
  SequenceTree TC = star_tree(sequence_names(alignments[0]));
  if (args.count("t-constraint"))
    TC = load_constraint_tree(args["t-constraint"].as<string>(),sequence_names(alignments[0]));

  T = TC;
  RandomTree(T,1.0);

  //-------------- Link --------------------------------//
  link(alignments,T,internal_sequences);

  //---------------process----------------//
  for(int i=0;i<alignments.size();i++) 
  {
    
    //---------------- Randomize alignment? -----------------//
    if (args.count("randomize-alignment"))
      alignments[i] = randomize(alignments[i],T.n_leaves());
  
    //------------------ Analyze 'internal'------------------//
    if ((args.count("internal") and args["internal"].as<string>() == "+")
	or args.count("randomize-alignment"))
      for(int column=0;column< alignments[i].length();column++) {
	for(int j=T.n_leaves();j<alignments[i].n_sequences();j++) 
	  alignments[i](column,j) = alphabet::not_gap;
      }

    //---- Check that internal sequence satisfy constraints ----//
    check_alignment(alignments[i],T,internal_sequences[i]);
  }
}

/// \brief Load a tree and an alignment based on command line parameters.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_A_and_T(const variables_map& args,alignment& A,RootedSequenceTree& T,bool internal_sequences)
{
  A = load_A(args,internal_sequences);

  T = load_T(args);

  //------------- Link Alignment and Tree -----------------//
  link(A,T,internal_sequences);

  //---------------- Randomize alignment? -----------------//
  if (args.count("randomize-alignment"))
    A = randomize(A,T.n_leaves());
  
  //------------------ Analyze 'internal'------------------//
  if ((args.count("internal") and args["internal"].as<string>() == "+")
      or args.count("randomize-alignment"))
    for(int column=0;column< A.length();column++) {
      for(int i=T.n_leaves();i<A.n_sequences();i++) 
	A(column,i) = alphabet::not_gap;
    }

  //---- Check that internal sequence satisfy constraints ----//
  check_alignment(A,T,internal_sequences);
}

/// \brief Load a tree and an alignment based on command line parameters.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_A_and_T(const variables_map& args,alignment& A,SequenceTree& T,bool internal_sequences)
{
  RootedSequenceTree RT;
  load_A_and_T(args,A,RT,internal_sequences);
  T = RT;
}

/// \brief Load an alignment based on command line parameters and generate a random tree.
///
/// \param args The command line parameters.
/// \param alignments The alignments.
/// \param T The leaf-labelled tree.
/// \param internal_sequences Should each resulting alignment have sequences for internal nodes on the tree?
/// 
void load_A_and_random_T(const variables_map& args,alignment& A,SequenceTree& T,bool internal_sequences)
{
  // NO internal sequences, yet!
  A = load_A(args,internal_sequences);

  //------------- Load random tree ------------------------//
  SequenceTree TC = star_tree(sequence_names(A));
  if (args.count("t-constraint"))
    TC = load_constraint_tree(args["t-constraint"].as<string>(),sequence_names(A));

  T = TC;
  RandomTree(T,1.0);

  //------------- Link Alignment and Tree -----------------//
  link(A,T,internal_sequences);

  //---------------- Randomize alignment? -----------------//
  if (args.count("randomize-alignment"))
    A = randomize(A,T.n_leaves());
  
  //------------------ Analyze 'internal'------------------//
  if ((args.count("internal") and args["internal"].as<string>() == "+")
      or args.count("randomize-alignment"))
    for(int column=0;column< A.length();column++) {
      for(int i=T.n_leaves();i<A.n_sequences();i++) 
	A(column,i) = alphabet::not_gap;
    }

  //---- Check that internal sequence satisfy constraints ----//
  check_alignment(A,T,internal_sequences);
}

/// Construct a multifurcating tree representing topology constraints from file \a filename.
///
/// \param filename The name of the file to load the tree from.
/// \param names The order of the leaf labels.
/// \return a multifurcating tree.
///
SequenceTree load_constraint_tree(const string& filename,const vector<string>& names)
{
  RootedSequenceTree RT;
  RT.read(filename);

  SequenceTree constraint = RT;
      
  remove_sub_branches(constraint);
  
  try{
    remap_T_indices(constraint,names);
  }
  catch(const bad_mapping<string>& b) {
    bad_mapping<string> b2(b.missing,b.from);
    if (b.from == 0)
      b2<<"Constraint tree leaf sequence '"<<b2.missing<<"' not found in the alignment.";
    else
      b2<<"Alignment sequence '"<<b2.missing<<"' not found in the constraint tree.";
    throw b2;
  }
  return constraint;
}

/// Return an OwnedPointer (possibly NULL) to an IndelModel or type \a name.
OwnedPointer<IndelModel> get_imodel(string name) 
{
  //-------------Choose an indel model--------------//
  OwnedPointer<IndelModel> imodel;

  // Default
  if (name == "") 
    name = "RS07";

  if (name == "none")
    { }
  else if (name == "RS05")
    imodel = SimpleIndelModel();
  else if (name == "RS07-no-T")
    imodel = NewIndelModel(false);
  else if (name == "RS07")
    imodel = NewIndelModel(true);
  else
    throw myexception()<<"Unrecognized indel model '"<<name<<"'";
  
  return imodel;
}

/// Parse the file $HOME/.bali-phy and add the options it contains to the command line arguments.
///
/// \param args The command line arguments.
/// \param options The allowed options.
///
void load_bali_phy_rc(variables_map& args,const options_description& options)
{
  if (fs::path::default_name_check_writable())
    fs::path::default_name_check(fs::portable_posix_name);

  if (getenv("HOME")) {
    string home_dir = getenv("HOME");
    if (not fs::exists(home_dir))
      cerr<<"Home directory '"<<home_dir<<"' does not exist!"<<endl;
    else if (not fs::is_directory(home_dir))
      cerr<<"Home directory '"<<home_dir<<"' is not a directory!"<<endl;
    else {
      string filename = home_dir + "/.bali-phy";

      if (fs::exists(filename)) {
	if (log_verbose)
	  cerr<<"Reading ~/.bali-phy ...";
	ifstream file(filename.c_str());
	if (not file)
	  throw myexception()<<"Can't load config file '"<<filename<<"'";
      
	store(parse_config_file(file, options), args);
	file.close();
	notify(args);
	if (log_verbose)
	  cerr<<" done."<<endl;
      }
    }
  }
  else
    cerr<<"Environment variable HOME not set!"<<endl;
}

/// \brief Parse a string of the form int,int,int:string 
///
/// \param model The string to parse.
/// \param partitions The list of integers.
/// \return the string.
///
string parse_partitions_and_model(string model, vector<int>& partitions)
{
  partitions.clear();

  int colon = model.find(':');
  if (colon == -1)
    return model;

  string prefix = model.substr(0,colon);
  model = model.substr(colon+1);

  partitions = split<int>(prefix,',');

  return model;
}

/// \brief Parse command line arguments of the form --key int,int,int:name1 --key int,int:name2
///
/// Here the integers refer to partitions 1..n and so cannot be referred to twice.
///
/// \param args The command line arguments.
/// \param key The key.
/// \param n The number of partitions that exist.
/// \return a mapping from partitions to names.
///
shared_items<string> get_mapping(const variables_map& args, const string& key, int n)
{
  vector<string> models;
  if (args.count(key))
    models = args[key].as<vector<string> >();

  vector<int> mapping(n,-2);
  vector<string> model_names;

  // If we just have --key=name, then each partition gets a separate version of 'name'
  if (models.size() == 1) 
  {
    vector<int> partitions;
    string model_name = parse_partitions_and_model(models[0],partitions);
      
    if (partitions.size() == 0) 
    {
      if (model_name == "none")
	mapping = vector<int>(n,-1);
      else 
	for(int i=0;i<mapping.size();i++)
	{
	  mapping[i] = i;
	  model_names.push_back(model_name);
	}
      return shared_items<string>(model_names,mapping);
    }
  }

  /// For each argument --key {int}+:name
  for(int i=0;i<models.size();i++) 
  {
    // 1. Parse {int}+:name into partitions and model_name
    vector<int> partitions;

    int index = model_names.size();
    string model_name = parse_partitions_and_model(models[i],partitions);
    if (model_name == "none")
      index = -1;
    else 
      model_names.push_back(model_name);

    // 2. Check that partitions have been specified ...
    if (partitions.size() == 0) 
    {
      // unless there is only one partition, or ...
      if (n == 1)
	partitions.push_back(1);
      else {
	assert(models.size() > 1);
	throw myexception()<<"Failed to specify partition number(s) for '"<<key<<"' specification '"<<models[i];
      }
    }

    // 3. Map partitions to this model, unless they are already mapped
    for(int j=0;j<partitions.size();j++) 
    {
      // Check for bad partition numbers.
      if (partitions[j] < 1 or partitions[j] > n)
	throw myexception()<<"Partition "<<partitions[j]<<" doesn't exist.";

      // Check for partition already mapped.
      if (mapping[partitions[j]-1] != -2)
	throw myexception()<<"Trying to set '"<<key<<"' for partition "<<partitions[j]<<" twice.";

      // Map the partition to this model.
      mapping[partitions[j]-1] = index;
    }
  }

  // Every unmentioned partition gets a mapping to a unique ""
  for(int i=0;i<mapping.size();i++)
    if (mapping[i] == -2) 
    {
      mapping[i] = model_names.size();
      model_names.push_back("");
    }

  return shared_items<string>(model_names,mapping);
}
