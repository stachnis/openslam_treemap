/*!
Copyright (c) 2009, Universitaet Bremen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the Universitaet Bremen nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef TMTREEMAP_H
#define TMTREEMAP_H

/*!\file tmTreemap.h 
   \brief Class TmTreemap
   \author Udo Frese
   
  Contains the class \c TmTreemap representing a
  a treemap with all algorithms involved.
*/

#include "tmTypes.h"
#include "tmNode.h"
#include "tmFeature.h"
#include <deque>
#include <vectormath/vectormath.h>
#include <stdexcept>

//! A treemap. The main classed used by an application to perform SLAM
/*!
 */
class TmTreemap 
{
 public:

  friend class MainWindow;
  friend class TmNode;  
  
  //! Uninitialised treemap
  TmTreemap();

  //! Copy constructor
  TmTreemap (const TmTreemap& tm);

  //! Std constructor
  TmTreemap (int nrOfMovesPerStep, int maxNrOfUnsuccessfulMoves);  

  //! Initializes an empty treemap
  /*! See \c Optimizer::movesPerStep and \c Optimizer::maxNrOfUnsuccessfulMoves. */
  void create (int nrOfMovesPerStep=4, int maxNrOfUnsuccessfulMoves=3);  

  //! Destructor
  virtual ~TmTreemap();

  //! Assignment
  TmTreemap& operator = (const TmTreemap& tm);
  
  //! Root node of the binary tree.
  TmNode* root;

  //! List of all nodes indexed by \c TmNode::index
  XycVector<TmNode*> node;

  //! List of unused node indices
  /*! When a new node is generated by \c newLeaf the index
      is taken from \c unusedNodes. 
  */
  XycVector<int> unusedNodes;  



  //! Whether the estimate and all internal data structures have been
  //! updated.
  bool isEstimateValid;  

  //! Whether the nodes \c gaussianValid flags reflect the current situation.
  /*! This flag is usually true. Only during the HTP optimization algorithm
      the systems moves nodes to variuous positions invalidating their 
      feature passed lists. Normally it would have to invalidate the gaussians
      too but it doesn't do that because mostly it moves the nodes just back.
      This is denoted by \c isGaussianValidValid=false so even Gaussians that
      are flagged valid are not actually valid.
   */
  bool isGaussianValidValid;  

  //! Whether the Gaussians have been updated
  /*! Implies \c isFeaturePassedValid() but not necessarily isEstimateValid().
   */
  bool isGaussianValid() const 
    {return isGaussianValidValid && (root==NULL || root->isFlag(TmNode::IS_GAUSSIAN_VALID));}  


  //! Whether the \c .featurePassed lists are valid.
  /*! When returning \c true all "static" information on which feature
      is represented where is valid. This includes \c
      TmNode::featurePassed lists and \c Feature::marginalizationNode.
  */
  bool isFeaturePassedValid () const
    {return root==NULL || root->isFlag(TmNode::IS_FEATURE_PASSED_VALID);}


  //! Reserves a block of \c n consecutive features
  /*! Returns the number of the first feature. The treemap maintains
      links of unused feature blocks of different size so getting such
      a block is efficient. If \c n is more than \c
      MAX_FEATURE_BLOCK_SIZE there is no feature block list and a
      block is appended to \c feature.  We retuns parts of larger
      blocks only if they are integer multiples of \c n to avoid
      fragmenting the space of feature indices.
   */
  virtual TmFeatureId newFeatureBlock (int n);

  
  //! Frees a feature
  /*! Frees the feature again. If features of a block are freed in
      consecutive order they are merged again to a free block.
  */
  virtual void deleteFeature (TmFeatureId id);  

  //! Prints information on reused feature numbers
  void printFeatureFragmentation () const;  


  //! Is called, when feature \c id is sparsified out during joining of leaves
  /*! The default implementation just does nothing. However
      any derived class can overload this function for its specific
      purposes. It can reset some \c TmFeature::CAN_BE_SPARSIFIED
      flags, thus implementing an application specific sparsification
      policy.

      Remember that sparsifying out a feature does not yet mean
      deleting it.  A feature is sparsified out if it is involved in
      several leaves and marginalized out of one leaf. This is an approximation
      and must be controlled to avoid loosing too much information. Still it
      is involved in the second leaf and only if it is marginalized out there,
      \c deleteFeature will be called.
   */
  virtual void hasBeenSparsifiedOut (TmFeatureId id);  
  
  
  //! Resets to no feature, no information
  virtual void clear();  



  //! Optimizes with full runs. Not spreading runs over several steps.
  virtual void optimizeFullRuns ();
  

  //! Return a textual description what happened in the last optimizations
  /*! The report is cleared so with each call one gets what happened
      since the last call.
  */
  string getAndClearReport ()
    {return optimizer.getAndClearReport();}
  


  //! Recomputes the whole tree from leaves to root
  /*! Invalidates \c IS_FEATURE_PASSED_VALID and \c IS_GAUSSIAN_VALID and
      recomputes.
  */
  void fullRecompute ();  

      

  //! A single move of one subtree to a different location
  /*! The \c oldAbove member is used to be able to undo the move. */
  class Move {
  public:
    //! Value of the cost function after moving \c subtree
    /*! This member variable can be used are can be left alone. */
    double cost;    

    //! Move the subtree below \c subtree around.
    TmNode* subtree;

    //! Which child was \c subtree before moving
    /*! This information is needed to exactly undo a move */
    int whichChild;    

    //! Now \c subtree is above \c oldAbove, i.e. \c oldAbove is its sibling.
    TmNode* oldAbove;

    //! And we move it to above \c above.
    /*! i.e. subtree->parent will be made the new parent of \c newAbove
        and \c subtree.
    */
    TmNode* above;

    //! Whether to join \c subtree and \c above
    /*! If this flag is \c tree \c subtree is moved to above
        \c above and then both are joined into a single leaf.
        (\c above and \c subtree must be leaves). Note, that
        this move cannot be undone.
     */
    bool join;    

       


    //! Empty move
    Move ()
      :cost(vmInf()), subtree(NULL), whichChild(-1), oldAbove(NULL), above(NULL), 
      join(false)
      {}

    //! Make empty
    void clear ();    

    //! Construct a move of \c subtree to \c above.
    Move (TmNode* subtree, TmNode* above=NULL)
      :cost(vmInf()), subtree(subtree), above(above), join(false)
      {
        setOldAbove();        
      }

    //! Returns whether this is a no-move move.
    bool isEmpty () const  {return above==NULL;}    

    //! Sets \c oldAbove and \c whichChild
    void setOldAbove ();    


    //! Preliminary execute the move
    /*! Sets \c oldAbove, move or joins but invalidates only \c
      IS_FEATURE_PASSED_VALID not \c IS_GAUSSIAN_VALID. So when the
      move is later revoked by \c undoIt the Gaussians are still valid
      and need not be recomputed. This routine is used when the
      algorithm tries how a move would affect the update cost. 
      Also reset the \c CAN_BE_MOVED flag.
    */
    void tryIt ();

    //! Permanently execute the move
    /*! Sets \c oldAbove, moves or joins and invalidates both \c
      IS_FEATURE_PASSED_VALID and \c IS_GAUSSIAN_VALID. This routine
      is used when the algorithm finally decides to do a move. 
    */
    void doIt ();    
    
    //! Move back and assert that \c above is sibling of \c subtree
    /*! Sets the \c CAN_BE_MOVED flag. */
    void undoIt ();
  };  


  //! Just stores \c \c Move::subtree->index and \c Move::above->index
  class MoveIndices
    {
    public:
      //! Move::subtree->index
      int subtreeIndex;
      //! Move::above->index
      int aboveIndex;    
      //! Move::oldAbove->index
      int oldAboveIndex;      

      //! Empty constructor
      MoveIndices ()
        :subtreeIndex(-1), aboveIndex(-1), oldAboveIndex(-1) {}
      //! Std constructor
      MoveIndices (int subtreeIndex, int aboveIndex, int oldAboveIndex)
        :subtreeIndex(subtreeIndex), aboveIndex(aboveIndex), oldAboveIndex (oldAboveIndex)
        {}      
    };  
  
      

  //! Finds the best node \c subtree to move to \c above from one side of \c lca to the other.
  /*! The cost function to be optimized is \c lca->worstCaseUpdateCost
      or in case one entire side of lca is moved the corresponding
      other child replacing \c lca in the tree. The algorithm considers only
      nodes which are marked \c CAN_BE_MOVED.
      
      Apart from moving subtrees the routine also considers to move
      and join (in one step) a single leaf. However it accepts these
      joining moves only as optimal if the resulting cost is 
      \c <joinOnlyBelow. The reason for this criterion is that we cannot
      undo joining operation so we only execute them in the KL
      optimization if they lead to actual improvement of the cost
      function.

      \c move.cost gives \c lca->worstCaseUpdateCost if \c subtree was
      moved to \c above. Note that this value can be larger than \c
      lca->worstCaseUpdateCost in which case the cost cannot be
      improved by a single step. Still the KL heuristic will take this
      step to see whether improvement is possible later on.

      This routine only computes the best move. It does not actually move anything
      and it does not update the optimizer state. So the optimizer \c Optimizer
      works by repeatedly finding a step with \c optimalKLStep, then executing the
      step and performing bookkeeping on the optimizer state.
   */
  void optimalKLStep (TmNode* lca, double joinOnlyBelow, Move& move); 


  //! Computes the same as \c optimalKernighanLinStep very slowly but safely
  /*! Note, while the cost returned is always the same as for \c optimalKLStep,
      the actual step return may be different in case there are several steps
      with the same cost function.
      
      \c specialMove is purely for debugging purposes and doesn't
      influence the computation. It allows to set a breakpoint when \c
      safeOptimalKLStep consideres \c specialMove (among all others)
      to see what happens.
  */
  void safeOptimalKLStep (TmNode* lca, double joinOnlyBelow, Move& move, const Move& specialMove);  


  //! Adds a new leaf with \c gaussian
  /*! The overall distribution represented is multiplied by \c
      gaussian.  Nodes are invalidated by not recomputed, so adding
      several leaves is efficient. See \c TmNode::Status. The new leaf
      is returned.

      The leaf is first inserted above the root but immediately moved
      to a reasonable location by \c recursiveOptimalDescend.
  */
  TmNode* addLeaf (const TmGaussian& gaussian, int flags=TmNode::CAN_BE_INTEGRATED);
  
  //! Adds \c newLeaf to the tree and invalidates accordingly.
  /*! This routine is for integrating probability distributions that
      shall be recomputed from some application specific non-Gaussian
      information. For doing so, the programmer must derive her/his
      subclass of TmNode, implement specific code to maintain the
      nonlinear distribution in mind and to compute a Gaussian
      approximation \c TmNode::Gaussian that is used within the
      framework. \c newLeaf must be created by the application but
      will be destroyed by \c TmTreemap.
   */
  void addNonlinearLeaf (TmNode* newLeaf);


  //! Sets the estimate for \c id to \c est, if it is not yet set.
  /*! Can be used to provide an initial estimate before update
      is called. The treemap algorithm only needs an initial
      estimates for those features used as linearization points.
      It takes care of setting these themself by using \c gaussian.linearizationPoint
      in \c addLeaf, \c addNonlinearLeaf and \c change.
  */
  void setInitialEstimate (TmFeatureId id, float est);  


  //! Global information for each feature
  /*! Includes estimate, flags describing the type of feature,
      a pointer to the node where it has been marginalized out
      and total number of original distributions where it has
      been involved in. Feature indices throughout the treemap
      are indices to this array. Take care to reserve enough
      memory to avoid copying.
  */
  XycVector<TmFeature> feature;

  //! We maintain lists of free blocks of features below this size
  enum {MAX_FEATURE_BLOCK_SIZE=16};
  
      
  //! Index to unused feature blocks of different size.
  /*! See \c newFeatureBlock. Obviously \c firstUnusedFeature[0]
      is not used.
   */
  int firstUnusedFeature[MAX_FEATURE_BLOCK_SIZE];


  //! Returns the node with \c index
  TmNode* getNode (int index) const
    {
      if (0<=index && index<(int) node.size()) return node[index];
      else return NULL;      
    }  


  //! Updates the \c TmNode::featurePassed lists.
  /*! Also updates \c Feature::marginalizationNode. Thus the "static"
      information, which feature is represented where is up to date,
      still not actual floating-point computation has been done,
      i.e. the Gaussians are still invalid and the estimate too.
  */
  void updateFeaturePassed ();  

  
  //! A single iteration of \c computeEstimate
  /*! Updates all Gaussians and computes an estimate. Does not call
      \c relinearize.

      Computation is performed by recursively pre-order traversing the
      tree. The Gaussian stored at a node is used to compute the
      estimate for the features involved in the Gaussian conditioned
      on the estimate passed down from the parent (\c
      TmGaussian::mean()). The result is passed to the children.

      This is the only O(n) step. It is however still very fast
      because only simple backsubstitution (resp. a matrix vector
      product) is needed in each node.

      Since \c TmTreemap cares only about Gaussians this is certainly
      a linear estimate. We explicitly call it linear because
      subclasses may overload \c computeNonlinearEstimate to employ an
      iterative scheme where in a loop the linearization point of some
      nodes is updated, the Gaussians recomputed and \c
      computeLinearEstimate is called.
  */
  virtual void computeLinearEstimate ();


  //! Allows to limit the update of estimates to \c from to \c to
  /*! Resets the DONT_UPDATE_ESTIMATE at least one node involving each
      feature \c [from..to-1]. If \c setDontUpdateFlag==true, it sets
      the DONT_UPDATE_ESTIMATE flag in all nodes before, so only the
      estimate for those nodes needed for \c [from..to-1] are updated.
   */
  void onlyUpdateEstimatesFor (int from, int to, bool setDontUpdateFlag=true);  

  //! Resets the \c DONT_UPDATE_ESTIMATE flag so all estimates get updated
  /*! Does not actually update the estimate. This is only done after calling
      \c computeLinearEstimate or \c computeNonlinearEstimate.
  */
  void updateAllEstimates ();  


  //! Update all Gaussians but not the estimate
  void updateGaussians ();


  //! Cost for updating all invalid Gaussians
  double updateGaussiansCost () const;  


  //! Computes a nonlinear estimate from the treemap's Gaussians
  /*! \c TmTreemap provides support for relinearization but does not
      implement one mechanism by itself, because all relinearization
      schemes depend on a particular meaning of the estimated random
      variables. Thus the routine simply calls \c
      computeEstimate. However it can be overloaded by a derived class
      to implemented some relinearization of nodes, i.e.  recomputing
      some Gaussians using the recent estimate as linearization point.

      Basically there are two approaches supported.  One can
      completely recompute the Gaussians of some leaves from some
      nonlinear original information stored at the leave in a class
      derived from \c TmNode (\c TmSlamDriver2DL::NonlinearLeaf as an
      example). Additionally one can use the \c
      TmGaussian::linearizationPoint mechanism to specifically address
      linearization errors of rotating information that should in
      theory be rotation invariant. This is implemented by the \c ??? 
      mechanism and implicitly used whenever a node is updated.
   */
  virtual void computeNonlinearEstimate ();

  //! Finds all leaves involving \c node
  /*! The routine is efficient because it descends from the marginalizationNode
      of \c id down thorugh nodes that represent \c id.
  */
  void findLeavesInvolving (TmFeatureId id, XycVector<TmNode*>& node);  

  //! Sparsifies features [id..id+n-1] out
  /*! Sets the \c CAN_BE_SPARSIFIED flag of these features and calls
      \c joinSubtree for all leaves involving feature \c id.
      Note that the \c n different features must be consecutive and
      if any of them is involved then \c id must be involved too.

      All features must be \c CAN_BE_MARGINALIZED_OUT
   */
  void sparsifyOut (TmFeatureId id, int n);  

  //! Whether \c id can be sparsified out
  /*! Sparsification implies loosing information, so it is application
      dependent to decide whether a feature can be sparsified
      out. Thus \c canBeSparsifiedOut must be overloaded by any
      derived class. Always a prerequisite is that the
      marginalizationNode of a feature must be optimized. This
      prevents premature sparsification. 
  */
  virtual bool canBeSparsifiedOut (TmFeatureId id);  

  //! Checks, whether features marginalized at \c n can be sparsified out
  /*! This routine is called by the optimizer whenever setting a node
      to \c TmNode::IS_OPTIMIZED. The default implementation is empty
      (no sparsification). A derived class must overload it (and \c
      canBeSparsifiedOut) to implement a specific sparsification
      policy. The routine must check, whether any feature marginalized
      out at \c n could be sparsified out. For all features that can
      it should call \c sparsifyOut.
   */
  virtual void checkForSparsification (TmNode* n);  


  //! Replaces feature \c assignment[i].first by \c assignment[i].second
  /*! This routine can be used for defered loop closing. Observed
      features are first integrated as new features. Then later an
      algorithm detect that such a feature is actually the same as a
      feature observed before.  Then the new feature id is replaced by
      the old one via \c identifyFeatures and this information is
      incorporated.
  */
  void identifyFeatures (const XycVector<pair<int, int> >& assignment);  

  //! Asserts the internal consistency of the unused feature list
  void assertUnusedFeatureLists () const;  

  //! Asserts some invariants on the tree
  virtual void assertIt () const;  


  //! Returns time since first call in seconds
  /*! Only implemented in LINUX. */
  static double time();

  //! Calibrates the computation time of making a Gaussian as a 3rd order polynomial in n
  /*! The polynomial fitted is 

      \code
          coef[0]+n*coef[1]+n*n*coef[2]+n*n*n*coef[3]. 
      \endcode
     
      By setting bit i of \c deactive,
      one can force the i-th coefficient to be 0.

      If \c filename is \c !=NULL, i (column 1), the raw data (column
      2) and fitted polynomial (column 3) are written to \c filename.

      Our result was (seconds) and fits reasonably well:

      \code
           1.543037E-6 + 1.154801E-6*n + 44.716E-9*n^2 + 1.799E-9*n^3
      \endcode
   */
  static void calibrateGaussianPerformance (int nMax, double coef[4], int deactive=0, char* filename=NULL);

  //! Returns, whether this code and the xymMatrix code has been compiled with all optimizations.
  static bool isCompiledWithOptimization();

  //! Counts the number of features for which an estimate is provided
  /*! Only those features are counted that have all \c mustFlag s set
    and no \c mayNotFlag. */
  int nrOfFeatures (int mustFlag=0, int mayNotFlag=0) const;  
    

  //! Problem statistics (nr. of landmarks/poses/etc.) for a SLAM problem
  /*! This class is only meaningful if an overloaded \c TmTreemap class is
      used that implements a SLAM algorithm.
  */
  class SlamStatistic 
  {
  public:
    //! Number of landmarks
    int n;
    //! Number of measurements
    int m;
    //! Number of robot poses
    int p;
    //! Number of robot poses that have been marginalized out (without loss of information)
    int pMarginalized;
    //! Number of robot poses that have been sparsified out (with loss of information)
    int pSparsified;

    SlamStatistic ()
      :n(0), m(0), p(0), pMarginalized(0), pSparsified(0)
      {}      

      //! Resets the statistics to 0
      void clear() {n = m = p = pMarginalized = pSparsified = 0; }      
  };
  

  //! Returns the number of landmarks, measurements and robot poses used.
  /*! \c TmTreemap just handles Gaussians treating everything as a
      plain 1-DOF random variable.  So it has no notion of landmarks,
      poses, etc and this function is not implemented. However, any
      derived class implementing a SLAM scenario can overload this
      function to return its own counters. \c n is the number of landmarks
      \c m of
   */
  virtual SlamStatistic slamStatistics () const;  

  //! Operational statistics of the treemap algorithm
  class TreemapStatistics 
  {
  public:
    //! Nr of used nodes in the tree (\c node[i]!=NULL)
    int nrOfNodes;  

    //! Nr of nodes in the optimization queue
    int nrOfNodesToBeOptimized;    

    //! Formal \c updateCost for all nodes that have been updated
    /*! Accumulated since initializing the treemap(). 
     */
    double accumulatedUpdateCost;  
    
    //! Number of nodes where the Gaussian has been updated
    long int nrOfGaussianUpdates;  
    
    //! Corresponding accumulated cost for \c optimalKLStep
    double accumulatedOptimizationCost;      

    class HTPEntry 
    {
    public:
      //! Nr of times we (did not) had success in optimizing
      int success, noSuccess;
      HTPEntry () :success(0), noSuccess(0){}        
    };
    

    //! For making an HTP statistic
    /*! \c htp[i] contains a statistic how often we were successful/unsuccessful in
        with the i-th KL step after the last improvement.
    */
    XycVector<HTPEntry> htp;

    //! Memory consumption in bytes
    int memory;    

    TreemapStatistics ()
      : nrOfNodes(0), nrOfNodesToBeOptimized(0),
      accumulatedUpdateCost(0), nrOfGaussianUpdates(0), accumulatedOptimizationCost (0), memory(0)
      {}      

      //! Tells the statistics, that we tried \c n step and whether we had success
      void optimizationStatistics (int n, bool success);  
      
      //! Probability according to statistic that we will still find an improvement after \c n unsuccessful steps.
      double optimizationCondProb (int n);  
  };  

  //! Returns the statistics as on treemap's operation as defined by \c TreemapStatistics
  /*! If \c expensive is \c true also entries that are expensive to compute are computed.
      This includes \c stat.mem. 
  */
  void computeStatistics ( TreemapStatistics& stat, bool expensive=true) const;
  
  
      
  //! Computes an estimate by plain QR (slow).
  void computeEstimateByQR ();

  //! Recomputes the estimate by plain QR and checks that it is the same.
  void assertEstimate ();  

  //! Returns the official name of feature \c featureId as a text
  /*! This routine is used to display list of features for debugging
      purposes. It should be overloaded by any specialized class
      implementing a specific SLAM model (2D, 3D, poses only, with
      features etc.). The text is written to \c txt (20 characters
      at most). Usually features are multi-dimensional. So the
      routine returns the dimension of that feature. Correspondingly
      \c [featureId..featureId+n-1] are all together represented
      by \c txt.
   */
  virtual void nameOfFeature (char* txt, int featureId, int& n) const;  

  //! Overloaded function that returns a string
  string nameOfFeature (int featureId);  


  //! Prints the Gaussian to stdout using feature names from \c nameOfFeature()
  void printGaussian (TmGaussian& g);  

  //! Total memory consumption of map (bytes) including nodes, etc.
  virtual int memory () const;  
  
 protected:
  //! Returns the update cost that \c subtree had if it was joined into one leaf
  double costOfJoining (TmNode* subtree);  

  //! Joins all original distributions below \c subtree in a single
  //! Gaussian 
  /*! All original distributions must be marked \c
      CAN_BE_INTEGRATED. All features that are marked \c
      CAN_BE_MARGINALIZED_OUT and not represented elsewhere are
      marginalized out permanently. Features marked \c
      CAN_BE_SPARSIFIED may be sparsified depending on circumstances.

      The worst case cost of \c subtree after it was/would be joined
      is returned in \c cost. As the cost (mostly) increases when the
      tree increases one can stop trying to join once the cost is
      above the root's worst case cost. This is essential since this
      routine searches through the whole subtree \c subtree thus
      needing too much computation time when applied to large trees.

      The routine is virtual so it can be overloaded by a derived class
      to update status information.
   */
  virtual void joinSubtree (TmNode* subtree);

  //! Determines the features involved in a joined distribution for \c subtree 
  /*! It does not change the tree itself. The tree must be \c
      IS_FEATURE_PASSED_VALID before calling. If any original
      distribution below \c subtree is not marked \c
      CAN_BE_INTEGRATED an empty list and \c nPM=nM=nP=-1 are returned.

      \c fl is the list of features involved in some distribution
      below \c subtree it is grouped into three parts.

       \c fl[0..nPM-1] are those features that will be marginalized
       out permantly. The algorithm will not compute an estimate for
       them anymore. This can be features that are marked \c
       CAN_BE_MARGINALIZED_OUT and not involved outside \c subtree. Or
       it can be features that are marked \c CAN_BE_SPARSIFIED even
       when they are involved outside subtree. 

       \c fl[nPM..nPM+nM-1] are those features that are still maintained
       by the algorithm but that will be marginalized out at \c subtree,
       retaining as usual the conditional distribution so an estimate can
       be commputed.

       \c fl[nPM+nM..nPM+nM+nP-1] are those features passed to the parent.

       So overall after triangularizing the Gaussian the first \c nPM
       rows/columns are discarded and the remaining columns can be
       used as usual.
  */
  void effectOfJoining (TmNode* subtree, TmExtendedFeatureList& fl, int& nPM, int& nM, int& nP) const;  


  //! Computes all features involved in leaves below \c subtree with counter added.
  void computeFeaturesInvolvedBelow (TmNode* subtree, TmExtendedFeatureList& fl);  
  

  //! Puts all inner nodes below \c into \c innerNode and all leaves into \c leaf
  void recursiveAllNodes (TmNode* n, XycVector<TmNode*>& innerNode, XycVector<TmNode*>& leaf);  
  
    

  //! Auxiliary routine for \c costOfCollapsing
  /*! Add all features involved below \c subtree to \c fl with duplicates
   */
  void recursivelyAdd (TmExtendedFeatureList& fl, TmNode* subtree) const;  

  //! Recursively stacks all input Gaussians below \c subtree into \c join
  void recursivelyMultiply (TmGaussian& join, TmNode* subtree);  

  //! Recursively deletes \c n and all ancestors.
  void recursivelyDelete (TmNode* n);

  //! Recursively subtracts all leaves below \c n from \c TmFeature::count
  void recursivelySubtractCount (TmNode* n);

  //! Recursively count the number of leaves involving a landmark \c i in \c count[i]
  void recursivelyCount (TmNode* n, XycVector<int>& count) const;

  //! Returns the least common ancestor of \c a and \c b.
  TmNode* lca (TmNode* a, TmNode* b) const;

  //! Auxiliary function for copy operator / constructor
  /*! Recursively copies the subtree below \c n2 and returns the copy
      of the subtree's root. Sets the \c .tree pointer to \c
      this. While copying it redirects all \c .marginalizationNode
      pointer in \c *this that still point to original nodes to the
      corresponding copy.
   */
  TmNode* recursiveCopyTreeFrom (const TmNode* n2);  



  
  //! Rotate \c gaussian by angle
  /*! Rotates the Gaussian by \c angle. This means, that the
      probability in the new distribution of x rotated by angle around
      (0,0) is the same as the probability of x in the old
      distribution.

      Conceptually this is routine should be a member of \c
      TmGaussian. However it is not because it must be implemented in an
      application dependent way and evaluated the feature flags to determine
      whether a feature is an x, y or z coordinate or an angle or whatever.
      
      Thus this routine depends on the concrete meaning of the different
      features, whereas the remaining algorithm just treats them as some
      random variables.

      ( We will have to think about this again. )
   */
  virtual void rotateGaussian (TmGaussian& gaussian, double angle) const;  

  //! Nr of features needed to specify a linearization point for exact rotation
  /*! Return 1  if the map is 2-D or 3-D with measured inclination.
      Return 3 for general 6-DOF SLAM. Compare \c rotateGaussian.
   */
  virtual int nrOfLinearizationPointFeatures (int feature) const 
  {feature=feature; /* to avoid warning */ return 1;}


  //! Subroutine for \c optimalKLStep
  /*! Recursively checks all nodes below \c subtreeBelow which must be
      below \c lca whether by moving them from one side of \c lca to
      the other will improve \c lca->worstCaseUpdateCost.  It
      considers only nodes that share at least one feature with \c
      lca. If it finds a transfer that is better than \c move.cost it
      replaces \c move. See \c recursiveOptimalDescend for \c joinOnlyBelow

      It correctly handles the case when \c lca disappears because \c subtree
      is one of its children.      
  */
  void recursiveOptimalKL (TmNode* lca, TmNode* subtreeBelow, int sideOfLca, double joinOnlyBelow, Move& bestMove);
  

  //! Finds the best position below \c bestMove.subtree->parent to move \c bestMove.subtree.
  /*! The routine optimizes \c .worstCaseUpdateCost of the least
      common ancestor of \c bestMove.subtree and \c
      bestMove.subtree->sibling(). In the current position this is \c
      bestMove.subtree->parent. However, when \c bestMove.subtree
      moves below \c bestMove.subtree->sibling(), \c
      bestMove.subtree->parent moves with it and \c
      bestMove.subtree->sibling() is the lca replacing \c
      bestMove.subtree->parent in the overall tree.

      \c The whole move including cost is returned in \c bestMove.

      If \c bestMove.subtree is a leaf the routine also considers
      moving the leaf and joining it with another leaf as a single
      step. However it returns this move only as optimal if the
      resulting cost is below \c joinOnlyBelow. The reason for this
      behavior is that joining cannot be undoed, so we accept it in
      the KL optimization only if it actually leads to an improved
      cost function.

      If \c mayStayHere==false, the option to leave \c bestMove.subtree
      where it is is forbidden.

      The routine considers only nodes for \c bestMove.above that
      share a landmark with \c bestMove.subtree. It further terminates
      the search if the cost is \c >=bound. 

      For two optimal solutions \c bestAbove1, \c bestAbove2 the
      routine chooses that one that leads to the smallest \c
      worstCaseUpdateCost for the lca of \c bestAbove1 and \c
      bestAbove2. This happens quite frequently if \c bestMove.subtree is not
      on the worst case path after insertion.

      The routine does not modify \c bestMove.subtree and \c bestMove.oldAbove.
   */
  void recursiveOptimalDescend (double bound, double joinOnlyBelow, Move& bestMove, bool mayStayHere=true);

  //! This class contains the state of the KL based HTP optimizer
  /*! The general strategy with KL is to greedily move the subtree that
      minimzes the cost function but to do this even if it leads to an
      increasing cost function. Later on following steps may lead to a
      cost function that is lower than the initial one, so this strategy
      allows to overcome local minima. 

      Treemap spreads this computation over several steps of the algorithm.
      
   */
  class Optimizer 
    {
    public:

      //! Empty constructor
      Optimizer ();
      
      //! Std constructor
      Optimizer (TmTreemap* tree, int nrOfMovesPerStep, int maxNrOfUnsuccessfulMoves);      
      
      //! The tree on which to operate
      TmTreemap* tree;

      //! FIFO queue of node indices that will be processed by the HTP subalgorithm
      /*! Contains all nodes that do not have \c IS_OPTIMIZED
        set. Whenever a nodes \c IS_OPTIMIZED flag is reset, the node is
        pushed to the back of the queue. \c optimize() takes nodes from
        the front. The queue may contain nodes that have already been optimized
        and it may contain nodes twice. These are ignored by \c optimize. 
      */
      deque<int> optimizationQueue;
      
      
      //! Index of the node, the worstCaseUpdateCost of which is optimized.
      /*! Originally \c lcaIndex is \c nextNodeToBeOptimized()->index. However
          some moves may lead to the node moved to a completely different position
          in the tree. In this case \c lcaIndex is replace by the node that takes
          the position that \c nextNodeToBeOptimized() originally had.

          If \c lcaIndex<0 this indicates that currently no node is
          being processed.
      */
      int lcaIndex;
      
      //! \c nextNodeToBeOptimized()->worstCaseUpdateCost before moving
      /*! If \c getNode(lcaIndex)<initialCost, the overall cost has been
          improved and the sequence of KL steps is confirmed and finished.
       */
      double initialCost;
      
      //! List of moves that did not decrease the cost below \c initialCost
      /*! We try \c maxNrOfUnsuccessfulMoves always appending the move to
          \c unsuccessfulMoves. If then we do not succeed in reducing the
          cost below \c initialCost, we undo all of them.
       */
      XycVector<MoveIndices> unsuccessfulMoves;

      //! Maximum number of unsuccessful moves to try before giving up
      int maxNrOfUnsuccessfulMoves; 

      //! Nr of KL moves executed per SLAM step
      /*! This is only the default value and can be overwritten by \c optimizeNSteps()
       */
      int nrOfMovesPerStep;      

      //! Textual description of what the optimizer did
      /*! In every optimization step the optimizer adds to \c report until
          it is \c 200 characters long. The application should take the
          report and clear it so new reports information can be appended.

          Reports are not generated if \c NDEBUG is defined.
      */
      string report;      

      //! Initialize
      void create (TmTreemap* treem, int nrOfMovesPerStep, int maxNrOfUnsuccessfulMoves);
      
      //! Perform a whole run of optimization
      /*! I.e. move nodes until there is either an improvement or \c
          maxNrOfUnsuccessfulMoves is reached. In the latter case undo
          all moves WITHOUT invalidating Gaussians.
       */
      void oneKLRun ();      

      //! Returns \c report and clears it.
      string getAndClearReport ();      

      //! Memory consumption in bytes
      int memory () const;      

    protected:
      //! Fetches the next node that should be optimized from \c optimizationQueue
      /*! Reads and removes nodes from \c optimizationQueue that have \c
        IS_OPTIMIZED flag set or that have invalid indices (may happen
        when a node is removed).  Then returns the first valid node but
        does NOT remove it from \c optimizationQueue. Sets \c lcaIndex
        and \c initialCost.

        During optimization the node to be optimized is still \c
        optimizationQueue.front(). It is only removed after the
        optimization is finished. See \c lcaIndex.
      */
      TmNode* nextNodeToBeOptimized ();
    };
  
  //! The state of the KL based HTP optimizer
  Optimizer optimizer;  

  //! Recursive internal function for \c updateGaussiansCost
  double recursiveUpdateGaussiansCost (const TmNode* n) const;  

  //! Statistics of treemap's algorithmic activity
  /*! \c mem and \c nrOfNodesToBeOptimized is kept 0 and computed by \c computeStatistics. */
  TreemapStatistics stat;  

  //! asserts that all leaves are connected by at least \c minDOF shared features
  /*! The routine defines two leaves as connected when they share at
      least \c minDOF features. This then computes the connected
      components according to this definition and asserts that there
      is only one. The implementation is not very efficient (no
      union-find and O(n^2) leaf vs. leaf comparison.).
   */
  void assertConnectivity (int minDOF) const;  

  //! This vector contains work space for the QR decomposition \c xymGEQR2 and \c TmGaussian::mean.
  /*! The memory remains allocated and is extended if necessary across different
      calls so we save the allocation and deallocation.
  */
  XymVector workspace;  

  //! This vector contains float work space for \c TmGaussian::meanCompressed.
  /*! The memory remains allocated and is extended if necessary across different
      calls so we save the allocation and deallocation.
  */
  XycVector<float> workspaceFloat;  
  
 protected:

  //! Assigns \c newNode->index
  /*! If there is an unused index in \c unusedNodes it takes one. Otherwise it appends
      an entry to \c node. \c node[newNode->index] is assigned \c newNode.
  */
  void newNodeIndex (TmNode* newNode);  
};


// Inline implementation of \c TmNode member. We need it here, because it
// references \c TmTreemap::isEstimateValid.
inline void TmNode::resetFlagUpToRoot (int flag)
{
  int nFlag = ~flag;
  TmNode* n = this;
  while (n!=NULL && (n->status & flag)!=0) {
    n->status &= nFlag;
    n = n->parent;
  }
  if ((flag & IS_GAUSSIAN_VALID)!=0) tree->isEstimateValid = false;
}

#endif
