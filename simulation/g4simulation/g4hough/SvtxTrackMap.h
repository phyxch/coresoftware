#ifndef __SVTXTRACKMAP_H__
#define __SVTXTRACKMAP_H__

#include "SvtxTrack.h"

#include <phool/PHObject.h>
#include <map>
#include <iostream>

class SvtxTrackMap : public PHObject {
  
public:

  typedef std::map<unsigned int, SvtxTrack*> TrackMap;
  typedef std::map<unsigned int, SvtxTrack*>::const_iterator ConstIter;
  typedef std::map<unsigned int, SvtxTrack*>::iterator            Iter;
  
  virtual ~SvtxTrackMap() {}
  
  virtual void identify(std::ostream& os = std::cout) const {
    os << "SvtxTrackMap base class" << std::endl;
  }
  virtual void Reset() {}
  virtual int  IsValid() const {return 0;}
  virtual SvtxTrackMap* Clone() const {return NULL;}
  
  virtual bool   empty()                   const {return true;}
  virtual size_t  size()                   const {return 0;}
  virtual size_t count(unsigned int idkey) const {return 0;}
  virtual void   clear()                         {}
  
  virtual const SvtxTrack* get(unsigned int idkey) const {return NULL;}
  virtual       SvtxTrack* get(unsigned int idkey) {return NULL;}
  virtual       SvtxTrack* insert(const SvtxTrack *cluster) {return NULL;}
  virtual       size_t     erase(unsigned int idkey) {return 0;}

  virtual ConstIter begin()                   const {return TrackMap().end();}
  virtual ConstIter  find(unsigned int idkey) const {return TrackMap().end();}
  virtual ConstIter   end()                   const {return TrackMap().end();}

  virtual Iter begin()                   {return TrackMap().end();}
  virtual Iter  find(unsigned int idkey) {return TrackMap().end();}
  virtual Iter   end()                   {return TrackMap().end();}

protected:
  SvtxTrackMap() {}
  
private:
    
  ClassDef(SvtxTrackMap, 1);
};

#endif
