/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b,int n, protocol proto )
{
   ulong i, j;   
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
   coherence_protocol=proto;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));
   cache_number= n;
  
   //*******************//
   //initialize your counters here//
   //*******************//
   num_cache_transfers  = 0;
   num_memory_transfers = 0;
   num_invalidations    = 0;
   num_interventions    = 0;
   num_flushes          = 0;
   num_busrd_x          = 0;
   num_bus_upgrade      = 0;
      
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

void cacheLine::setFlagsModified(protocol coherence_proto)
{
   if(coherence_proto==MSI || coherence_proto==MESI)
   {
      setFlags(MODIFIED);
   }
   // else if(coherence_proto == Dragon)
   // {
   //    if(getFlags()==Sc)
   //    {
   //       setFlags(Sm);
   //    }
   //    else
   //    {
   //       setFlags(MODIFIED);
   //    }      
   // }
   else
   {
      setFlags(MODIFIED);
   }    
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr,uchar op,vector<Cache> *cacheArr)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') 
   {      
      writes++;
   }
	else
   {
      reads++;
   }
	
	cacheLine * line = findLine(addr);
   bool if_copy_exists = checkC(addr,cacheArr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') 
      {
         if(coherence_protocol==MSI || coherence_protocol==MESI)
         {
            busrdx(addr,cacheArr);
            writeMisses++;
         }
         if(coherence_protocol==Dragon)
         {
            busrd(addr,cacheArr);
            if(if_copy_exists)
            {
               busupdate(addr,cacheArr);
            }
            writeMisses++;
         }
      }
		else 
      {
         busrd(addr,cacheArr);
         readMisses++;
      }
		cacheLine *newline = fillLine(addr);
      
      if(op == 'w') 
      {
         newline->setFlagsModified(coherence_protocol);   
         if(coherence_protocol==Dragon && if_copy_exists)
         {
            newline->setFlags(Sm);
         }                 
      }
      else if(op == 'r' && coherence_protocol==MESI && !if_copy_exists)	
      {
         newline->setFlags(EXCLUSIVE);
      }
      else if(op == 'r' && coherence_protocol==Dragon)
      {
         if(if_copy_exists)
         {
            newline->setFlags(Sc);
         }
         else
         {
            newline->setFlags(EXCLUSIVE);
         }         
      }
      
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') 
      {
         if(coherence_protocol==MSI && line->getFlags()==SHARED)
         {
            busrdx(addr,cacheArr);
         }
         else if(coherence_protocol==MESI && line->getFlags()==SHARED)
         {            
            busupgrade(addr,cacheArr);
         } 
         else if(coherence_protocol==Dragon && (line->getFlags()==Sc || line->getFlags()==Sm))
         {            
            busupdate(addr,cacheArr);
         }  

         
         if(coherence_protocol==Dragon && if_copy_exists) 
         {
            line->setFlags(Sm);
         }else
         {
            line->setFlagsModified(coherence_protocol);  
         }
         
         
      }
	}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	   if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(coherence_protocol==Dragon && victim->getFlags()==Sm) writeBack(addr);
   else if(victim->getFlags() == MODIFIED) writeBack(addr);
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   if(coherence_protocol==MSI || coherence_protocol==MESI)
   {
      victim->setFlags(SHARED);
   }
   else
   {
      victim->setFlags(SHARED);
   }
   
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

bool Cache::checkC(ulong addr, vector<Cache> *ca)
{
   uint i=0;   
   for(vector<Cache>::iterator it= ca->begin(); it != ca->end(); it++,i++)
	{
      if(i!=cache_number)
      {
         cacheLine* target = it->findLine(addr);	
         if(target!=NULL)
			{
            return true;     
         }
      }
   }
   return false;
}

void Cache::busrd(ulong addr, vector<Cache> *ca)
{
   uint i=0;
   if(coherence_protocol==MSI || coherence_protocol==Dragon)
   {
      num_memory_transfers++;
   }
   
   bool cache_to_cache_transfer_done=false;
   for(vector<Cache>::iterator it= ca->begin(); it != ca->end(); it++,i++)
	{
      if(i!=cache_number)
      {
         cacheLine* target = it->findLine(addr);	
         if(coherence_protocol==MSI)
			{
            if(target != NULL && target->getFlags()==MODIFIED)
            {
               // cout<<"In here busrd\n";
               it->incrementFlushes();
               it->incrementMemoryTransactions();
               target->setFlags(SHARED);
               it->incrementInterventions();
            }
         }
         else if(coherence_protocol==MESI)
         {
            if(target != NULL && (target->getFlags()==EXCLUSIVE || target->getFlags()==MODIFIED))
            {
               it->incrementInterventions();
               if(target->getFlags()==MODIFIED)
               {
                  it->incrementFlushes();
                  it->incrementMemoryTransactions();
                  cache_to_cache_transfer_done=true;
                  (*ca)[cache_number].incrementCacheTransfers();
               }
            }
            if(target != NULL && (target->getFlags()==EXCLUSIVE || target->getFlags()==SHARED))
            {                             
               if(!cache_to_cache_transfer_done)
               {
                  cache_to_cache_transfer_done=true;
                  (*ca)[cache_number].incrementCacheTransfers();
               }
            }
            if(target!=NULL)
            {               
               target->setFlags(SHARED); 
            }            
         }
         else if(coherence_protocol==Dragon)
         {

            if(target != NULL && (target->getFlags()==Sm || target->getFlags()==MODIFIED))
            {
               it->incrementFlushes();
               it->writeBacks--;
               //it->incrementMemoryTransactions();
            }
            if(target != NULL && (target->getFlags()==EXCLUSIVE || target->getFlags()==MODIFIED))
            { 
               it->incrementInterventions();
               if(target->getFlags()==EXCLUSIVE)
               {
                  target->setFlags(Sc);   
               }
               else if(target->getFlags()==MODIFIED)
               {
                  target->setFlags(Sm);  
               }               
            }
         }
      }
   }
   if(coherence_protocol==MESI && !cache_to_cache_transfer_done)
   {
      num_memory_transfers++;
   }
}

void Cache::busrdx(ulong addr, vector<Cache> *ca)
{
   if(coherence_protocol==MSI || coherence_protocol==Dragon)
   {
      num_memory_transfers++;
   }
   // num_memory_transfers++;
   num_busrd_x++;
   assert(coherence_protocol!=Dragon);
	uint i=0;
   bool cache_to_cache_transfer_done=false;
	for(vector<Cache>::iterator it= ca->begin(); it != ca->end(); it++,i++)
	{
      
      if(i!=cache_number)
		{
         cacheLine* target = it->findLine(addr);	
         if(target != NULL && (coherence_protocol==MSI || coherence_protocol==MESI))
         {		
            // cout<<"In here busrdx\n";		
            if(target->getFlags() == MODIFIED)
            {            
               it->incrementFlushes();
               it->incrementMemoryTransactions();
               cache_to_cache_transfer_done=true;
               if(coherence_protocol==MESI)
               {
                  (*ca)[cache_number].incrementCacheTransfers();
               }               
            }
            if(coherence_protocol==MESI && (target->getFlags()==SHARED||target->getFlags()==EXCLUSIVE) && !cache_to_cache_transfer_done)
            {
               cache_to_cache_transfer_done=true;
               (*ca)[cache_number].incrementCacheTransfers();
            }
            target->invalidate();	
            it->incrementInvalidations();		
         }
      }		
	}
   if(coherence_protocol==MESI && !cache_to_cache_transfer_done)
   {
      num_memory_transfers++;
   }
}

void Cache::busupgrade(ulong addr, vector<Cache> *ca)
{
   //This should never be hit if the target is Exclusive or Modified
   num_bus_upgrade++;
   uint i=0;   
	for(vector<Cache>::iterator it= ca->begin(); it != ca->end(); it++,i++)
	{
		if(i!=cache_number)
		{
         cacheLine* target = it->findLine(addr);	
         assert(coherence_protocol!=MSI);
			if(target != NULL && (coherence_protocol==MESI ))
			{
            // assert(target->getFlags()!=MODIFIED && target->getFlags()!=EXCLUSIVE);	
            // cout<<"In here busupgrade\n";	            
				target->invalidate();	
            it->incrementInvalidations();
			}
		}
		
	}
}

void Cache::busupdate(ulong addr, vector<Cache> *ca)
{
   //This should never be hit if the target is Exclusive or Modified   
   uint i=0;   
	for(vector<Cache>::iterator it= ca->begin(); it != ca->end(); it++,i++)
	{
		if(i!=cache_number)
		{
         cacheLine* target = it->findLine(addr);	
         // assert(target->getFlags()!=MODIFIED && target->getFlags()!=EXCLUSIVE);
         assert(coherence_protocol!=MSI);
			if(target != NULL && (coherence_protocol == Dragon) && target->getFlags()==Sm)
			{            	
            // cout<<"In here busupdate\n";	            
				target->setFlags(Sc);	            
			}
		}
	}
}

void Cache::printStats()
{   
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
   // printf("%lu\n",getReads());
   // printf("%lu\n",getRM());
   // printf("%lu\n",getWrites());
   // printf("%lu\n",getWM());
   // printf("%.2f%%\n",getMissRate());
   // printf("%lu\n",getWB());
   // printf("%lu\n",getCacheTransfers());
   // printf("%lu\n",getMemoryTransactions());
   // printf("%lu\n",getInterventions());
   // printf("%lu\n",getInvalidations());
   // printf("%lu\n",getFlushes());
   // printf("%lu\n",getBusRdX());


	printf("============ Simulation results (Cache %lu) ============\n",cache_number);
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
   printf("01. number of reads:				%lu\n",getReads());
   printf("02. number of read misses:			%lu\n",getRM());
   printf("03. number of writes:				%lu\n",getWrites());
   printf("04. number of write misses:			%lu\n",getWM());
   printf("05. total miss rate:				%.2f%%\n",getMissRate());
   printf("06. number of writebacks:			%lu\n",getWB());
   printf("07. number of cache-to-cache transfers:		%lu\n",getCacheTransfers());
   printf("08. number of memory transactions:		%lu\n",getMemoryTransactions());
   printf("09. number of interventions:			%lu\n",getInterventions());
   printf("10. number of invalidations:			%lu\n",getInvalidations());
   printf("11. number of flushes:				%lu\n",getFlushes());
   printf("12. number of BusRdX:				%lu\n",getBusRdX());
   

}
