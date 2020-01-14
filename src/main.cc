/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <vector>
using namespace std;

#include "cache.h"

vector<string> protocol_definition{"MSI","MESI","Dragon"};

int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	enum protocol input_protocol   = static_cast<protocol>(atoi(argv[5]));	 /*0:MSI, 1:MESI, 2:Dragon*/
	string fname(argv[6]);

	printf("===== 506 Personal information =====\n");
	printf("Bibin Kurian George\n");
	printf("bkgeorge\n");
	printf("ECE492 Students? NO\n");
	printf("===== 506 SMP Simulator configuration =====\n");
	printf("L1_SIZE: %d\n",cache_size);
	printf("L1_ASSOC: %d\n",cache_assoc);
	printf("L1_BLOCKSIZE: %d\n", blk_size);
	printf("NUMBER OF PROCESSORS: %d\n", num_processors);
	printf("COHERENCE PROTOCOL: %s\n", protocol_definition[input_protocol].c_str());
	printf("TRACE FILE: %s\n",fname.c_str());	
	//****************************************************//	
	//*******print out simulator configuration here*******//
	//****************************************************//

 
	//*********************************************//
    //*****create an array of caches here**********//
	vector<Cache> cacheArray;
	cacheArray.reserve(num_processors);	
	
	for(int i=0;i<num_processors;i++)
	{				
		cacheArray.push_back(Cache(cache_size,cache_assoc,blk_size,i,input_protocol));
	}
	//*********************************************//	

	pFile = fopen (fname.c_str(),"r");
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//
	char * line = NULL;
    size_t len = 0;
	ssize_t read;    
	while((read = getline(&line, &len, pFile)) != -1)
	{
		string line_string(line);
		int first_space=line_string.find(" ");
		string proc_num_string = line_string.substr(0,first_space);
		int proc_num = atoi(proc_num_string.c_str());
		char op=line_string[first_space+1];
		ulong address = stoul(line_string.substr(first_space+3),nullptr,16);
		cacheArray[proc_num].Access(address,op,&cacheArray);		
	}
	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//
	for(int i=0;i<num_processors;i++)
	{
		cacheArray[i].printStats();		
	}
	
	
}
