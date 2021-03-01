
#ifndef __CPUI_HERITAGE__
#define __CPUI_HERITAGE__

#include "types.h"
#include "address.hh"

/// \brief Label for describing extent of address range that has been heritaged
struct SizePass {
  int4 size;			///< Size of the range (in bytes)
  int4 pass;			///< Pass when the range was heritaged
};

class LocationMap {
public:
  /// Iterator into the main map
  typedef map<Address,SizePass>::iterator iterator;
private:
  map<Address,SizePass> themap;	///< Heritaged addresses mapped to range size and pass number
public:
  iterator add(Address addr,int4 size,int4 pass,int4 &intersect); ///< Mark new address as \b heritaged
  iterator find(const Address &addr);			///< Look up if/how given address was heritaged
  int4 findPass(const Address &addr) const;		///< Look up if/how given address was heritaged
  void erase(iterator iter) { themap.erase(iter); }	///< Remove a particular entry from the map
  iterator begin(void) { return themap.begin(); }	///< Get starting iterator over heritaged ranges
  iterator end(void) { return themap.end(); }		///< Get ending iterator over heritaged ranges
  void clear(void) { themap.clear(); }			///< Clear the map of heritaged ranges
};

#endif
