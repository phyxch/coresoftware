#ifndef PHG4CYLINDERCELLCONTAINER_H__
#define PHG4CYLINDERCELLCONTAINER_H__

#include "PHG4CylinderCell.h"

#include <phool/PHObject.h>

#include <map>
#include <set>

class PHG4CylinderCellContainer: public PHObject
{

  public:
  typedef std::map<PHG4CylinderCellDefs::keytype,PHG4CylinderCell *> Map;
  typedef Map::iterator Iterator;
  typedef Map::const_iterator ConstIterator;
  typedef std::pair<Iterator, Iterator> Range;
  typedef std::pair<ConstIterator, ConstIterator> ConstRange;
  typedef std::set<int>::const_iterator LayerIter;
  typedef std::pair<LayerIter, LayerIter> LayerRange;

  PHG4CylinderCellContainer();

  virtual ~PHG4CylinderCellContainer() {}

  void Reset();

  void identify(std::ostream& os = std::cout) const;

  ConstIterator AddCylinderCell(const unsigned int detid, PHG4CylinderCell *newcylinderCell);
  
  //! preferred removal method, key is currently the cell id
  void RemoveCylinderCell(PHG4CylinderCellDefs::keytype key) {
    cellmap.erase(key);
  }

  //! inefficent, use key where possible instead
  void RemoveCylinderCell(PHG4CylinderCell *cell)
  {
    Iterator its = cellmap.begin();
    Iterator last = cellmap.end();
    for (; its != last;)
      {
	if (its->second == cell)
	  {
	    cellmap.erase(its++);
	  }
	else
	  {
	    ++its;
	  }
      }
  }


  Iterator findOrAddCylinderCell(PHG4CylinderCellDefs::keytype key);

  PHG4CylinderCellDefs::keytype genkey(const unsigned int detid);

  //! return all cylinderCells matching a given detid
  ConstRange getCylinderCells(const unsigned int detid) const;

  //! return all hist
  ConstRange getCylinderCells( void ) const;

  PHG4CylinderCell* findCylinderCell(PHG4CylinderCellDefs::keytype key);

  unsigned int size( void ) const
  { return cellmap.size(); }
  unsigned int num_layers(void) const
  { return layers.size(); }
  LayerRange getLayers() const
  { return make_pair(layers.begin(), layers.end());}

  double getTotalEdep() const;

 protected:
  Map cellmap;
  std::set<int> layers; // layers is not reset since layers must not change event by event

  ClassDef(PHG4CylinderCellContainer,1)
};

#endif
