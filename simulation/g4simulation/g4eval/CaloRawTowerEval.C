
#include "CaloRawTowerEval.h"
#include "CaloTruthEval.h"

#include <phool/getClass.h>
#include <phool/PHCompositeNode.h>
#include <g4cemc/RawTowerContainer.h>
#include <g4cemc/RawTower.h>
#include <g4detectors/PHG4CylinderCellContainer.h>
#include <g4detectors/PHG4CylinderCell.h>
#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4TruthInfoContainer.h>
#include <g4main/PHG4Particle.h>

#include <string>
#include <cstdlib>
#include <set>
#include <map>
#include <float.h>
#include <cassert>

using namespace std;

CaloRawTowerEval::CaloRawTowerEval(PHCompositeNode* topNode, std::string caloname)
  : _caloname(caloname),
    _trutheval(topNode,caloname),
    _towers(NULL),
    _g4cells(NULL),
    _g4hits(NULL),
    _truthinfo(NULL),
    _strict(false),
    _verbosity(1),
    _errors(0),
    _do_cache(true),
    _cache_all_truth_hits(),
    _cache_all_truth_primaries(),
    _cache_max_truth_primary_by_energy(),
    _cache_all_towers_from_primary(),
    _cache_best_tower_from_primary(),
    _cache_get_energy_contribution_primary() {
  get_node_pointers(topNode);
}

CaloRawTowerEval::~CaloRawTowerEval() {
  if (_verbosity > 0) {
    if ((_errors > 0)||(_verbosity > 1)) {
      cout << "CaloRawTowerEval::~CaloRawTowerEval() - Error Count: " << _errors << endl;
    }
  }
}

void CaloRawTowerEval::next_event(PHCompositeNode* topNode) {

  _cache_all_truth_hits.clear();
  _cache_all_truth_primaries.clear();
  _cache_max_truth_primary_by_energy.clear();
  _cache_all_towers_from_primary.clear();
  _cache_best_tower_from_primary.clear();
  _cache_get_energy_contribution_primary.clear();

  _trutheval.next_event(topNode);
  
  get_node_pointers(topNode);
}

std::set<PHG4Hit*> CaloRawTowerEval::all_truth_hits(RawTower* tower) {

  if (_strict) {assert(tower);}
  else if (!tower) {++_errors; return std::set<PHG4Hit*>();}
  
  if (_do_cache) {
    std::map<RawTower*,std::set<PHG4Hit*> >::iterator iter =
      _cache_all_truth_hits.find(tower);
    if (iter != _cache_all_truth_hits.end()) {
      return iter->second;
    }
  }

  std::set<PHG4Hit*> truth_hits;

  // loop over all the towered cells
  std::pair< std::map<unsigned int,float>::const_iterator,
	     std::map<unsigned int,float>::const_iterator > cell_range = tower->get_g4cells();
  for (std::map<unsigned int, float>::const_iterator cell_iter = cell_range.first;
       cell_iter != cell_range.second; ++cell_iter) {
    unsigned int cell_id = cell_iter->first;
    PHG4CylinderCell *cell = _g4cells->findCylinderCell(cell_id);

    if (_strict) {assert(cell);}
    else if (!cell) {++_errors; continue;}
    
    // loop over all the g4hits in this cell
    for (PHG4CylinderCell::EdepConstIterator hit_iter = cell->get_g4hits().first;
	 hit_iter != cell->get_g4hits().second;
	 ++hit_iter) {      
      PHG4Hit* g4hit = _g4hits->findHit(hit_iter->first);

      if (_strict) {assert(g4hit);}
      else if (!g4hit) {++_errors; continue;}
      
      // fill output set
      truth_hits.insert(g4hit);
    }
  }

  if (_do_cache) _cache_all_truth_hits.insert(make_pair(tower,truth_hits));
  
  return truth_hits;
}
  
std::set<PHG4Particle*> CaloRawTowerEval::all_truth_primaries(RawTower* tower) {

  if (_strict) {assert(tower);}
  else if (!tower) {++_errors; return std::set<PHG4Particle*>();}
  
  if (_do_cache) {
    std::map<RawTower*,std::set<PHG4Particle*> >::iterator iter =
      _cache_all_truth_primaries.find(tower);
    if (iter != _cache_all_truth_primaries.end()) {
      return iter->second;
    }
  }
  
  std::set<PHG4Particle*> truth_primaries;
  
  std::set<PHG4Hit*> g4hits = all_truth_hits(tower);

  for (std::set<PHG4Hit*>::iterator iter = g4hits.begin();
       iter != g4hits.end();
       ++iter) {
    PHG4Hit* g4hit = *iter;
    PHG4Particle* primary = get_truth_eval()->get_primary_particle( g4hit );

    if (_strict) {assert(primary);}
    else if (!primary) {++_errors; continue;}

    truth_primaries.insert(primary);
  }

  if (_do_cache) _cache_all_truth_primaries.insert(make_pair(tower,truth_primaries));
  
  return truth_primaries;
}

PHG4Particle* CaloRawTowerEval::max_truth_primary_by_energy(RawTower* tower) {

  if (_strict) {assert(tower);}
  else if (!tower) {++_errors; return NULL;}
  
  if (_do_cache) {
    std::map<RawTower*,PHG4Particle*>::iterator iter =
      _cache_max_truth_primary_by_energy.find(tower);
    if (iter != _cache_max_truth_primary_by_energy.end()) {
      return iter->second;
    }
  }
  
  // loop over all primaries associated with this tower and
  // get the energy contribution for each one, record the max
  PHG4Particle* max_primary = NULL;
  float max_e = FLT_MAX*-1.0;
  std::set<PHG4Particle*> primaries = all_truth_primaries(tower);

  for (std::set<PHG4Particle*>::iterator iter = primaries.begin();
       iter != primaries.end();
       ++iter) {

    PHG4Particle* primary = *iter;

    if (_strict) {assert(primary);}
    else if (!primary) {++_errors; continue;}
    
    float e = get_energy_contribution(tower,primary);
    if (isnan(e)) continue;
    if (e > max_e) {
      max_e = e;
      max_primary = primary;      
    }
  }

  if (_do_cache) _cache_max_truth_primary_by_energy.insert(make_pair(tower,max_primary));
  
  return max_primary;
}

std::set<RawTower*> CaloRawTowerEval::all_towers_from(PHG4Particle* primary) { 

  if (_strict) {assert(primary);}
  else if (!primary) {++_errors; return std::set<RawTower*>();}
  
  if (!_trutheval.is_primary(primary)) return std::set<RawTower*>();

  // use primary map pointer
  primary = get_truth_eval()->get_primary_particle(primary);

  if (_strict) {assert(primary);}
  else if (!primary) {++_errors; return std::set<RawTower*>();}
  
  if (_do_cache) {
    std::map<PHG4Particle*,std::set<RawTower*> >::iterator iter =
      _cache_all_towers_from_primary.find(primary);
    if (iter != _cache_all_towers_from_primary.end()) {
      return iter->second;
    }  
  }
  
  std::set<RawTower*> towers;
  
  // loop over all the towers
  for (RawTowerContainer::Iterator iter = _towers->getTowers().first;
       iter != _towers->getTowers().second;
       ++iter) {

    RawTower* tower = iter->second;

    // loop over all truth particles connected to this tower
    std::set<PHG4Particle*> primaries = all_truth_primaries(tower);
    for (std::set<PHG4Particle*>::iterator jter = primaries.begin();
	 jter != primaries.end();
	 ++jter) {
      PHG4Particle* candidate = *jter;

      if (_strict) {assert(candidate);}
      else if (!candidate) {++_errors; continue;}

      if (get_truth_eval()->are_same_particle(candidate,primary)) {
	towers.insert(tower);
      }    
    }
  }

  if (_do_cache) _cache_all_towers_from_primary.insert(make_pair(primary,towers));
  
  return towers;
}

RawTower* CaloRawTowerEval::best_tower_from(PHG4Particle* primary) {

  if (_strict) {assert(primary);}
  else if (!primary) {++_errors; return NULL;}
  
  if (!_trutheval.is_primary(primary)) return NULL;

  primary = get_truth_eval()->get_primary_particle(primary);

  if (_strict) {assert(primary);}
  else if (!primary) {++_errors; return NULL;}
  
  if (_do_cache) {
    std::map<PHG4Particle*,RawTower*>::iterator iter =
      _cache_best_tower_from_primary.find(primary);
    if (iter != _cache_best_tower_from_primary.end()) {
      return iter->second;
    }
  }
  
  RawTower* best_tower = NULL;
  float best_energy = FLT_MAX*-1.0;  
  std::set<RawTower*> towers = all_towers_from(primary);
  for (std::set<RawTower*>::iterator iter = towers.begin();
       iter != towers.end();
       ++iter) {
    RawTower* tower = *iter;

    if (_strict) {assert(tower);}
    else if (!tower) {++_errors; continue;}
    
    float energy = get_energy_contribution(tower,primary);
    if (isnan(energy)) continue;
    if (energy > best_energy) {
      best_tower = tower;
      best_energy = energy;
    }
  }
 
  if (_do_cache) _cache_best_tower_from_primary.insert(make_pair(primary,best_tower));
  
  return best_tower;
}

// overlap calculations
float CaloRawTowerEval::get_energy_contribution(RawTower* tower, PHG4Particle* primary) {

  if (_strict) {
    assert(tower);
    assert(primary);
  } else if (!tower||!primary) {
    ++_errors;
    return NAN;
  }
  
  if (!_trutheval.is_primary(primary)) return NAN;

  // reduce cache misses by using only pointer from PrimaryMap
  primary = get_truth_eval()->get_primary_particle(primary);

  if (_strict) {assert(primary);}
  else if (!primary) {++_errors; return NAN;}
  
  if (_do_cache) {
    std::map<std::pair<RawTower*,PHG4Particle*>, float>::iterator iter =
      _cache_get_energy_contribution_primary.find(make_pair(tower,primary));
    if (iter != _cache_get_energy_contribution_primary.end()) {
      return iter->second;
    }
  }
  
  float energy = 0.0;

  std::set<PHG4Particle*> g4particles = all_truth_primaries(tower);
  if (g4particles.find(primary) != g4particles.end()) {
  
    std::set<PHG4Hit*> g4hits = all_truth_hits(tower);
    for (std::set<PHG4Hit*>::iterator iter = g4hits.begin();
	 iter != g4hits.end();
	 ++iter) {
      PHG4Hit* g4hit = *iter;
      PHG4Particle* candidate = get_truth_eval()->get_primary_particle(g4hit);

      if (_strict) {assert(candidate);}
      else if (!candidate) {++_errors; continue;}

      if (get_truth_eval()->are_same_particle(candidate,primary)) {
	energy += g4hit->get_edep();
      }
    }
  }

  if (_do_cache) _cache_get_energy_contribution_primary.insert(make_pair(make_pair(tower,primary),energy));
  
  return energy;
}

void CaloRawTowerEval::get_node_pointers(PHCompositeNode *topNode) {

  // need things off of the DST...
  std::string towername = "TOWER_CALIB_" + _caloname;
  _towers = findNode::getClass<RawTowerContainer>(topNode,towername.c_str());
  if (!_towers) {
    cerr << PHWHERE << " ERROR: Can't find " << towername << endl;
    exit(-1);
  }
  
  std::string cellname = "G4CELL_" + _caloname;
  _g4cells = findNode::getClass<PHG4CylinderCellContainer>(topNode,cellname.c_str());
  if (!_g4cells) {
    cerr << PHWHERE << " ERROR: Can't find " << cellname << endl;
    exit(-1);
  }

  std::string hitname = "G4HIT_" + _caloname;  
  _g4hits = findNode::getClass<PHG4HitContainer>(topNode,hitname.c_str());
  if (!_g4hits){ 
    cerr << PHWHERE << " ERROR: Can't find " << hitname << endl;
    exit(-1);
  }

  _truthinfo = findNode::getClass<PHG4TruthInfoContainer>(topNode,"G4TruthInfo");
  if (!_truthinfo) {
    cerr << PHWHERE << " ERROR: Can't find G4TruthInfo" << endl;
    exit(-1);
  }

  return;
}
