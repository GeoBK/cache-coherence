/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum{
	INVALID = 0,
	//VALID,
   SHARED,
	MODIFIED,   
   EXCLUSIVE,
   Sm,
   Sc
};

enum protocol {
   MSI=0,
   MESI,
   Dragon,
   None
};



class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; 
 
public:
   cacheLine()            { tag = 0; Flags = 0; }
   ulong getTag()         { return tag; }
   ulong getFlags()			{ return Flags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(ulong flags)			{  Flags = flags;}
   void setFlagsModified(protocol coherence_proto);
   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }//useful function
   bool isValid()         { return ((Flags) != INVALID); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;


   //******///
   //add coherence counters here///
   //******///
   ulong num_cache_transfers,num_memory_transfers,num_interventions,num_invalidations,num_flushes,num_busrd_x,num_bus_upgrade;
   ulong cache_number;
   protocol coherence_protocol;
   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  
     
    Cache(int,int,int,int,protocol);
   //~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM(){return readMisses;} ulong getWM(){return writeMisses;} 
   ulong getReads(){return reads;}ulong getWrites(){return writes;}
   ulong getWB(){return writeBacks;}
   float getMissRate(){return (((float)readMisses+(float)writeMisses)/((float)reads+(float)writes))*100.00;}
   
   void writeBack(ulong)   
   {
      writeBacks++;
      incrementMemoryTransactions();
   }
   
   //void flush(ulong)   {writeBacks++;}
   void Access(ulong,uchar,vector<Cache>*);
   void printStats();
   void updateLRU(cacheLine *);

   //******///
   //add other functions to handle bus transactions///
   //******///
   ulong getCacheTransfers(){return num_cache_transfers;} ulong getMemoryTransactions(){return num_memory_transfers;} 
   ulong getInterventions(){return num_interventions;}ulong getInvalidations(){return num_invalidations;}
   ulong getFlushes(){return num_flushes;}ulong getBusRdX(){return num_busrd_x;}
   ulong getBusUpgrade(){return num_bus_upgrade;}

   void busrdx(ulong addr, vector<Cache> *ca);
   void busupgrade(ulong addr, vector<Cache> *ca);
   void busupdate(ulong addr, vector<Cache> *ca);
   void busrd(ulong addr, vector<Cache> *ca);
   bool checkC(ulong addr, vector<Cache> *ca);
      
   void incrementCacheTransfers()
   {
      num_cache_transfers++;
   } 
   void incrementMemoryTransactions()
   {
      num_memory_transfers++;
   } 
   void incrementInterventions()
   {
      num_interventions++;
   }
   void incrementInvalidations()
   {
      num_invalidations++;
   }
   void incrementFlushes()
   {
      num_flushes++;
      writeBacks++;
   }
   void incrementBusRdX()
   {
      num_busrd_x++;
   }
};

#endif
