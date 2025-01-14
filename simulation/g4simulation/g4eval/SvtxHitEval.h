
#ifndef __SVTXHITEVAL_H__
#define __SVTXHITEVAL_H__

#include "SvtxTruthEval.h"

#include <phool/PHCompositeNode.h>
#include <g4hough/SvtxHitMap.h>
#include <g4hough/SvtxHit.h>
#include <g4detectors/PHG4CylinderCellContainer.h>
#include <g4detectors/PHG4CylinderCell.h>
#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4TruthInfoContainer.h>
#include <g4main/PHG4Particle.h>

#include <set>
#include <map>

class SvtxHitEval {

public:

  SvtxHitEval(PHCompositeNode *topNode);
  virtual ~SvtxHitEval();

  void next_event(PHCompositeNode *topNode);
  void do_caching(bool do_cache) {
    _do_cache = do_cache;
    _trutheval.do_caching(do_cache);
  }
  void set_strict(bool strict) {
    _strict = strict;
    _trutheval.set_strict(strict);
  }
  void set_verbosity(int verbosity) {
    _verbosity = verbosity;
    _trutheval.set_verbosity(verbosity);
  }
  
  // access the clustereval (and its cached values)
  SvtxTruthEval* get_truth_eval() {return &_trutheval;}
  
  PHG4CylinderCell* get_cell(SvtxHit* hit);
  
  // backtrace through to PHG4Hits
  std::set<PHG4Hit*> all_truth_hits          (SvtxHit* hit);
  PHG4Hit*           max_truth_hit_by_energy (SvtxHit* hit);
  
  // backtrace through to PHG4Particles
  std::set<PHG4Particle*> all_truth_particles          (SvtxHit* hit);
  PHG4Particle*           max_truth_particle_by_energy (SvtxHit* hit);

  // forwardtrace through to SvtxHits
  std::set<SvtxHit*> all_hits_from(PHG4Particle* truthparticle);
  std::set<SvtxHit*> all_hits_from(PHG4Hit* truthhit);
  SvtxHit*           best_hit_from(PHG4Hit* truthhit);
  
  // overlap calculations
  float get_energy_contribution (SvtxHit* svtxhit, PHG4Particle* truthparticle);
  float get_energy_contribution (SvtxHit* svtxhit, PHG4Hit* truthhit);

  unsigned int get_errors() {return _errors + _trutheval.get_errors();}
  
private:

  void get_node_pointers(PHCompositeNode *topNode);

  SvtxTruthEval _trutheval;
  SvtxHitMap* _hitmap;
  PHG4CylinderCellContainer* _g4cells_svtx;
  PHG4CylinderCellContainer* _g4cells_tracker;
  PHG4HitContainer* _g4hits_svtx;
  PHG4HitContainer* _g4hits_tracker;
  PHG4TruthInfoContainer* _truthinfo;

  bool _strict;
  int _verbosity;
  unsigned int _errors;
  
  bool                                              _do_cache;
  std::map<SvtxHit*,std::set<PHG4Hit*> >            _cache_all_truth_hits;
  std::map<SvtxHit*,PHG4Hit*>                       _cache_max_truth_hit_by_energy;
  std::map<SvtxHit*,std::set<PHG4Particle*> >       _cache_all_truth_particles;
  std::map<SvtxHit*,PHG4Particle* >                 _cache_max_truth_particle_by_energy;
  std::map<PHG4Particle*,std::set<SvtxHit*> >       _cache_all_hits_from_particle;
  std::map<PHG4Hit*,std::set<SvtxHit*> >            _cache_all_hits_from_g4hit;
  std::map<PHG4Hit*,SvtxHit*>                       _cache_best_hit_from_g4hit;
  std::map<std::pair<SvtxHit*,PHG4Particle*>,float> _cache_get_energy_contribution_g4particle;
  std::map<std::pair<SvtxHit*,PHG4Hit*>,float>      _cache_get_energy_contribution_g4hit;
};

#endif // __SVTXHITEVAL_H__
