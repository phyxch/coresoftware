#ifndef RAWTOWERCONTAINER_H__
#define RAWTOWERCONTAINER_H__

#include "RawTowerDefs.h"
#include <phool/PHObject.h>
#include <phool/phool.h>
#include <iostream>
#include <map>

class RawTower;

class RawTowerContainer : public PHObject 
{

 public:

  typedef std::map<RawTowerDefs::keytype ,RawTower *> Map;
  typedef Map::iterator Iterator;
  typedef Map::const_iterator ConstIterator;
  typedef std::pair<Iterator, Iterator> Range;
  typedef std::pair<ConstIterator, ConstIterator> ConstRange;

  RawTowerContainer() {}
  virtual ~RawTowerContainer() {}

  void Reset();
  int isValid() const;
  void identify(std::ostream& os=std::cout) const;
  ConstIterator AddTower(const unsigned int ieta, const unsigned int iphi, RawTower *twr);
  ConstIterator AddTower(RawTowerDefs::keytype key, RawTower *twr);
  RawTower *getTower(RawTowerDefs::keytype key);
  RawTower *getTower(const unsigned int ieta, const unsigned int iphi);
  //! return all towers
  ConstRange getTowers( void ) const;
  Range getTowers( void );

  unsigned int size() const {return _towers.size();}
  void compress(const double emin);
  double getTotalEdep() const;
  RawTowerDefs::keytype genkey(const unsigned int ieta, const unsigned int iphi) const;

 protected:
  Map _towers;

  ClassDef(RawTowerContainer,1)
};

#endif /* RAWTOWERCONTAINER_H__ */
