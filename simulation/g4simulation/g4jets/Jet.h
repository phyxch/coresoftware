#ifndef __JET_H__
#define __JET_H__

#include <phool/PHObject.h>
#include <map>
#include <cmath>
#include <iostream>

class Jet : public PHObject {

public:

  // enums can be extended with new values, but values not altered
  
  enum ALGO {NONE=0, ANTIKT=1, KT=2, CAMBRIDGE=3};

  enum SRC {
    VOID=0,
    PARTICLE=1,
    TRACK=2,
    CEMC_TOWER=3, CEMC_CLUSTER=4,
    HCALIN_TOWER=5, HCALIN_CLUSTER=6,
    HCALOUT_TOWER=7, HCALOUT_CLUSTER=8,
  };

  enum PROPERTY {prop_JetCharge = 1,prop_BFrac = 2};

  Jet();
  virtual ~Jet() {}

  virtual void identify(std::ostream& os = std::cout) const;
  virtual void Reset() {return;}
  virtual int  isValid() const {return 0;}
  virtual Jet* Clone() const {return NULL;}

  // jet info ------------------------------------------------------------------
  
  virtual unsigned int get_id() const {return 0xFFFFFFFF;}
  virtual void         set_id(unsigned int id) {return;}

  virtual float get_px() const {return NAN;}
  virtual void  set_px(float px) {return;}

  virtual float get_py() const {return NAN;}
  virtual void  set_py(float py) {return;}

  virtual float get_pz() const {return NAN;}
  virtual void  set_pz(float pz) {return;}

  virtual float get_e() const {return NAN;}
  virtual void  set_e(float e) {return;}

  virtual float get_p() const {return NAN;}
  virtual float get_pt() const {return NAN;}
  virtual float get_et() const {return NAN;}
  virtual float get_eta() const {return NAN;}
  virtual float get_phi() const {return NAN;}
  virtual float get_mass() const {return NAN;}
  
  // extended jet info ---------------------------------------------------------

  virtual bool  has_property(Jet::PROPERTY prop_id) const {return false;}
  virtual float get_property(Jet::PROPERTY prop_id) const {return NAN;}
  virtual void  set_property(Jet::PROPERTY prop_id, float value) {return;}
  virtual void  print_property(ostream& os) const {return;}

  // component id storage ------------------------------------------------------
  
  /*! \addtogroup clustered component
   * clustered component methods (multimap interface based)
   * source type id --> unique id within that storage
   *  @{
   */

  typedef std::multimap<Jet::SRC, unsigned int> typ_comp_ids;
  typedef typ_comp_ids::const_iterator ConstIter;
  typedef typ_comp_ids::iterator Iter;

  virtual bool   empty_comp() const {return true;}
  virtual size_t size_comp() const {return 0;}
  virtual size_t count_comp(Jet::SRC source) const {return 0;}

  virtual void   clear_comp() {return;}
  virtual void   insert_comp(Jet::SRC source, unsigned int compid) {return;}
  virtual size_t erase_comp(Jet::SRC source) {return 0;}
  virtual void   erase_comp(Iter iter) {return;}
  virtual void   erase_comp(Iter first, Iter last) {return;}

  virtual ConstIter begin_comp() const {return typ_comp_ids().end();}
  virtual ConstIter lower_bound_comp(Jet::SRC source) const {return typ_comp_ids().end();}
  virtual ConstIter upper_bound_comp(Jet::SRC source) const {return typ_comp_ids().end();}
  virtual ConstIter find(Jet::SRC source) const {return typ_comp_ids().end();}
  virtual ConstIter end_comp() const {return typ_comp_ids().end();}

  virtual Iter begin_comp() {return typ_comp_ids().end();}
  virtual Iter lower_bound_comp(Jet::SRC source) {return typ_comp_ids().end();}
  virtual Iter upper_bound_comp(Jet::SRC source) {return typ_comp_ids().end();}
  virtual Iter find(Jet::SRC source) {return typ_comp_ids().end();}
  virtual Iter end_comp() {return typ_comp_ids().end();}

  /*! @} */
  
  ClassDef(Jet, 1);
};

#endif
