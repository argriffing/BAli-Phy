all: bali-phy

#----------------- Definitions
LI=${CXX}
EXACTFLAGS = -pipe

LANGO   = fast-math tracer prefetch-loop-arrays omit-frame-pointer
DEBUG   = # g3 pg 
DEFS    = NDEBUG_UBLAS NDEBUG_DP NDEBUG
WARN    = all no-sign-compare overloaded-virtual strict-aliasing
OPT     = march=pentium4 O3
LDFLAGS = #-pg -static

#------------------- Main 
PROGNAME = bali-phy
SOURCES = sequence.C tree.C alignment.C substitution.C moves.C \
          rng.C exponential.C eigenvalue.C parameters.C likelihood.C mcmc.C \
	  choose.C sequencetree.C sample-branch-lengths.C arguments.C \
	  util.C randomtree.C alphabet.C smodel.C bali-phy.C \
	  sample-tri.C hmm.C dp-engine.C 3way.C 2way.C sample-alignment.C \
	  sample-node.C imodel.C 5way.C sample-topology-NNI.C \
	  setup.C rates.C matcache.C sample-two-nodes.C sequence-format.C \
	  util-random.C alignment-random.C setup-smodel.C sample-topology-SPR.C \
	  alignment-sums.C alignment-util.C

LIBS = gsl gslcblas m 
LINKLIBS = ${LIBS:%=-l%}

${PROGNAME} : ${SOURCES:%.C=%.o} ${LINKLIBS}

tools/model_P: tools/statistics.o rng.o arguments.o ${LINKLIBS} 

tools/statreport: tools/statistics.o

tools/alignment-blame: alignment.o arguments.o alphabet.o sequence.o util.o rng.o \
	tree.o sequencetree.o tools/optimize.o tools/findroot.o tools/alignmentutil.o \
	setup.o smodel.o rates.o exponential.o eigenvalue.o sequence-format.o \
	alignment-random.o alignment-util.o randomtree.o ${LINKLIBS}

tools/alignment-reorder: alignment.o arguments.o alphabet.o sequence.o util.o rng.o \
	tree.o sequencetree.o tools/optimize.o tools/findroot.o setup.o smodel.o \
	rates.o exponential.o eigenvalue.o sequence-format.o randomtree.o ${LINKLIBS}

tools/alignment-draw: tree.o alignment.o sequencetree.o arguments.o \
	alphabet.o sequence.o sequence-format.o util.o setup.o rng.o\
	randomtree.o alignment-random.o ${LINKLIBS} 

tools/alignment-translate: alignment.o alphabet.o sequence.o arguments.o sequence-format.o \
	util.o	

tools/findalign: alignment.o alphabet.o arguments.o sequence.o tools/alignmentutil.o \
	rng.o ${LINKLIBS} util.o sequence-format.o

tools/treecount: tree.o sequencetree.o arguments.o util.o rng.o tools/statistics.o ${LIBS:%=-l%}

tools/tree-dist-compare: tree.o sequencetree.o tools/tree-dist.o arguments.o util.o \
	 rng.o tools/statistics.o ${LIBS:%=-l%}

tools/tree-dist-autocorrelation: tree.o sequencetree.o sequencetree.o arguments.o tools/tree-dist.o

tools/tree-to-srq: tree.o sequencetree.o arguments.o

tools/tree-names-trunc: tree.o sequencetree.o arguments.o util.o

tools/tree-reroot: tree.o sequencetree.o arguments.o

tools/srq-to-plot: arguments.o

tools/srq-analyze: arguments.o rng.o tools/statistics.o ${LIBS:%=-l%}

tools/make_random_tree: tree.o sequencetree.o arguments.o util.o\
	 rng.o  ${LIBS:%=-l%}

tools/phy_to_fasta: alignment.o sequence.o arguments.o alphabet.o \
	rng.o util.o sequence-format.o ${LIBS:%=-l%}

tools/analyze_distances: alignment.o alphabet.o sequence.o arguments.o\
	util.o sequencetree.o substitution.o eigenvalue.o tree.o sequencetree.o \
	parameters.o exponential.o setup-smodel.o smodel.o imodel.o rng.o likelihood.o \
	dpmatrix.o choose.o tools/optimize.o inverse.o setup.o rates.o matcache.o \
	sequence-format.o randomtree.o ${LINKLIBS}

tools/truckgraph: alignment.o arguments.o alphabet.o sequence.o util.o rng.o ${LIBS:%=-l%}

tools/truckgraph2: alignment.o arguments.o alphabet.o sequence.o util.o \
		tools/alignmentutil.o rng.o ${LINKLIBS}

tools/truckgraph3d: alignment.o arguments.o alphabet.o sequence.o util.o rng.o ${LIBS:%=-l%}

#------------------- End

SHELL = /bin/sh
MAKEFILES = GNUmakefile

includes += ./include/
includes += .

CC=gcc-3.4
CXX=g++-3.4
CPP = $(CC) -E  	# This might vary from machine to machine
CPPFLAGS = $(patsubst %,-I%,$(subst :, ,$(includes)))
CXXFLAGS = ${LANGO:%=-f%} ${WARN:%=-W%} ${DEBUG:%=-%} ${OPT:%=-%} ${DEFS:%=-D%} ${EXACTFLAGS}


GNUmakefile : ${SOURCES:%=.%.d} 

% : %.o
	${LI} ${LDFLAGS} $^ -o $@ ${LOADLIBS}

.%.d : %
	@echo ${shell \
	  ${CPP} -MM ${CPPFLAGS} $< | sed 's/\(^.*\):/$@ \1:/g'} > $@
clean:
	-@rm -f *.o *~ *# *.tar *.gz ${PROGNAME} Makefile core

# we are missing sources in the tools/ directory ...
ALLSOURCES=${SOURCES}
# -include ${ALLSOURCES:%=.%.d}