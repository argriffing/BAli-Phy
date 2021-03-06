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

#include "sample.H"
#include "util.H"
#include "rng.H"
#include <algorithm>
#include "mcmc.H"
#include "3way.H"
#include "likelihood.H"
#include "util-random.H"
#include "monitor.H"
#include "alignment-util.H"

using MCMC::MoveStats;

using std::valarray;
using std::vector;

void slide_node_move(Parameters& P, MoveStats& Stats,int b) 
{
  if (not P.smodel_full_tree)
    return;

  slide_node(P,Stats,b);
}

void change_branch_length_move(Parameters& P, MoveStats& Stats,int b) {
  if (not P.smodel_full_tree and b>=P.T->n_leaves())
    return;

  change_branch_length(P,Stats,b);
}

void change_branch_length_multi_move(Parameters& P, MoveStats& Stats,int b) {
  if (not P.smodel_full_tree and b>=P.T->n_leaves())
    return;

  change_branch_length_multi(P,Stats,b);
}

void sample_tri_one(Parameters& P, MoveStats&,int b) 
{
  const SequenceTree& T = *P.T;

  int node1 = T.branch(b).target();
  int node2 = T.branch(b).source();

  if (myrandomf() < 0.5)
    std::swap(node1,node2);

  if (node1 < T.n_leaves())
    std::swap(node1,node2);
    
  tri_sample_alignment(P,node1,node2);
}

void sample_tri_branch_one(Parameters& P, MoveStats& Stats,int b) 
{
  if (not P.smodel_full_tree and b>=P.T->n_leaves())
    return;

  MCMC::Result result(2);

  assert(P.variable_alignment()); 

  const SequenceTree& T = *P.T;

  int node1 = T.branch(b).target();
  int node2 = T.branch(b).source();

  if (myrandomf() < 0.5)
    std::swap(node1,node2);

  if (node1 < T.n_leaves())
    std::swap(node1,node2);
    
  const double sigma = 0.3/2;
  double length1 = T.branch(b).length();
  double length2 = length1 + gaussian(0,sigma);
  if (length2 < 0) length2 = -length2;

  if (tri_sample_alignment_branch(P,node1,node2,b,1,length2)) {
    result.totals[0] = 1;
    result.totals[1] = std::abs(length2 - length1);
  }

  Stats.inc("sample_tri_branch",result);
}


void sample_tri_branch_type_one(Parameters& P, MoveStats& Stats,int b) 
{
  if (not P.smodel_full_tree and b>=P.T->n_leaves())
    return;

  MCMC::Result result(1);

  assert(P.variable_alignment()); 

  const SequenceTree& T = *P.T;

  int node1 = T.branch(b).target();
  int node2 = T.branch(b).source();

  if (myrandomf() < 0.5)
    std::swap(node1,node2);

  if (node1 < T.n_leaves())
    std::swap(node1,node2);
    
  if (tri_sample_alignment_branch_model(P,node1,node2)) {
    result.totals[0] = 1;
  }

  Stats.inc("sample_tri_branch_type",result);
}


void sample_alignments_one(Parameters& P, MoveStats&,int b) {
  assert(P.variable_alignment()); 

  sample_alignment(P,b);
}

void sample_node_move(Parameters& P, MoveStats&,int node) {
  assert(P.variable_alignment()); 

  sample_node(P,node);
}

void sample_two_nodes_move(Parameters& P, MoveStats&,int n0) {
  assert(P.variable_alignment()); 

  vector<int> nodes = A3::get_nodes_random(*P.T,n0);
  int n1 = -1;
  for(int i=1;i<nodes.size();i++)
    if ((*P.T)[ nodes[i] ].is_internal_node()) {
      n1 = nodes[i];
      break;
    }
  assert(n1 != 1);

  int b = P.T->branch(n0,n1);

  sample_two_nodes(P,b);
}

vector<int> get_cost(const Tree& T) {
  vector<int> cost(T.n_branches()*2,-1);
  vector<const_branchview> stack1; stack1.reserve(T.n_branches()*2);
  vector<const_branchview> stack2; stack2.reserve(T.n_branches()*2);
  for(int i=0;i<T.n_leaves();i++) {
    const_branchview b = T.directed_branch(i).reverse();
    cost[b] = 0;
    stack1.push_back(b);
  }
    
  while(not stack1.empty()) {
    // fill 'stack2' with branches before 'stack1'
    stack2.clear();
    for(int i=0;i<stack1.size();i++)
      append(stack1[i].branches_before(),stack2);

    // clear 'stack1'
    stack1.clear();

    for(int i=0;i<stack2.size();i++) {
      vector<const_branchview> children;
      append(stack2[i].branches_after(),children);

      assert(children.size() == 2);
      int cost_l = cost[children[0]];
      int cost_r = cost[children[1]];
      if (cost_l != -1 and cost_r != -1) {
	if (not children[0].is_leaf_branch()) cost_l++;

	if (not children[1].is_leaf_branch()) cost_r++;

	if (cost_l > cost_r)
	  std::swap(cost_l,cost_r);

	cost[stack2[i]] = 2*cost_l + cost_r;
	stack1.push_back(stack2[i]);
      }
    }
  }
  
  // check that all the costs have been calculated
  for(int i=0;i<cost.size();i++)
    assert(cost[i] != -1);

  return cost;
}

vector<int> walk_tree_path(const Tree& T,int root) {

  vector<int> cost = get_cost(T);

  vector<int> tcost = cost;
  for(int i=0;i<cost.size();i++)
    tcost[i] += T.edges_distance(T.directed_branch(i).target(),root);

  vector<const_branchview> b_stack;
  b_stack.reserve(T.n_branches());
  vector<const_branchview> branches;
  branches.reserve(T.n_branches());
  vector<const_branchview> children;
  children.reserve(3);

  // get a leaf with minimum 'tcost'
  int leaf = 0;
  leaf = myrandom(T.n_leaves());
  for(int b=0;b<T.n_leaves();b++)
    if (tcost[T.directed_branch(b)] < tcost[T.directed_branch(leaf)])
      leaf = b;

  assert(T.directed_branch(leaf).source() == leaf);
  b_stack.push_back(T.directed_branch(leaf));

  while(not b_stack.empty()) {
    // pop stack into list
    branches.push_back(b_stack.back());
    b_stack.pop_back();

    // get children of the result
    children.clear();
    append(branches.back().branches_after(),children);
    children = randomize(children);

    // sort children in decrease order of cost
    if (children.size() < 2)
      ;
    else {
      if (children.size() == 2) {
	if (cost[children[0]] < cost[children[1]])
	  std::swap(children[0],children[1]);
      }
      else
	std::abort();
    }
      
    // put children onto the stack
    b_stack.insert(b_stack.end(),children.begin(),children.end());
  }

  assert(branches.size() == T.n_branches());

  vector<int> branches2(branches.size());
  for(int i=0;i<branches.size();i++)
    branches2[i] = branches[i].undirected_name();

  return branches2;
}

void sample_branch_length_(Parameters& P,  MoveStats& Stats, int b)
{
  //    std::clog<<"Processing branch "<<b<<" with root "<<P.LC.root<<endl;

  double slice_fraction = loadvalue(P.keys,"branch_slice_fraction",0.9);

  bool do_slice = (uniform() < slice_fraction);
  if (do_slice)
    slice_sample_branch_length(P,Stats,b);
  else
    change_branch_length(P,Stats,b);
    
  // Find a random direction of this branch, conditional on pointing to an internal node.
  const_branchview bv = P.T->directed_branch(b);
  if (uniform() < 0.5)
    bv = bv.reverse();
  if (bv.target().is_leaf_node())
    bv = bv.reverse();
  // NOTE! This pointer might be invalidated after the tree is changed by MH!
  //       We would modify T2 and then do T=T2, thus using the copied structue and destroying the original.

  // FIXME - this might move the accumulator off of the current branch (?)
  // TEST and Check Scaling of # of branches peeled
  if (myrandomf() < 0.5)
    slide_node(P,Stats,bv);
  else 
    change_3_branch_lengths(P,Stats,bv.target());

  if (not do_slice) {
    change_branch_length(P,Stats,b);
    change_branch_length(P,Stats,b);
  }
}

void walk_tree_sample_NNI_and_branch_lengths(Parameters& P, MoveStats& Stats) 
{
  vector<int> branches = walk_tree_path(*P.T, P[0].LC.root);

  for(int i=0;i<branches.size();i++)
  {
    int b = branches[i];

    double U = uniform();

    if (U < 0.1)
      slice_sample_branch_length(P,Stats,b);

    if (P.T->branch(b).is_internal_branch()) 
    {
      // In theory the 3-way move should have twice the acceptance rate, when the branch length
      // is non-zero, and one of the two other topologies is good while one is bad.
      //
      // This seems to actually occur for the Enolase-48 data set.
      if (myrandomf() < 0.95)
	three_way_topology_sample(P,Stats,b);
      else
	two_way_NNI_sample(P,Stats,b);
    }

    if (U > 0.9)
      slice_sample_branch_length(P,Stats,b);
  }
}


void walk_tree_sample_NNI(Parameters& P, MoveStats& Stats)
{
  vector<int> branches = walk_tree_path(*P.T, P[0].LC.root);

  for(int i=0;i<branches.size();i++) 
  {
    int b = branches[i];
    if (myrandomf() < 0.5)
      three_way_topology_sample(P,Stats,b);
    else
      two_way_NNI_sample(P,Stats,b);
  }
}


void walk_tree_sample_NNI_and_A(Parameters& P, MoveStats& Stats) 
{
  vector<int> branches = walk_tree_path(*P.T, P[0].LC.root);

  for(int i=0;i<branches.size();i++) 
  {
    int b = branches[i];
    if (myrandomf() < 0.01)
      three_way_topology_and_alignment_sample(P,Stats,b);
    else
      if (myrandomf() < 0.95)
	three_way_topology_sample(P,Stats,b);
      else
	two_way_NNI_sample(P,Stats,b);
  }
}


void walk_tree_sample_alignments(Parameters& P, MoveStats& Stats) 
{
  vector<int> branches = walk_tree_path(*P.T, P[0].LC.root);

  for(int i=0;i<branches.size();i++) {
    int b = branches[i];

    //    std::clog<<"Processing branch "<<b<<" with root "<<P.LC.root<<endl;

    if ((myrandomf() < 0.15) and (P.T->n_leaves() >2))
      sample_tri_one(P,Stats,b);
    else
      sample_alignments_one(P,Stats,b);
  }
}

void walk_tree_sample_branch_lengths(Parameters& P, MoveStats& Stats) 
{
  vector<int> branches = walk_tree_path(*P.T, P[0].LC.root);

  for(int i=0;i<branches.size();i++) 
  {
    int b = branches[i];

    // Do a number of changes near branch @b
    sample_branch_length_(P,Stats,b);
  }
}
