#include <iostream>
#include "Bucket.h"
#include <unordered_map>
#include "HashFunction.h"
#include "DensifiedMinHash.h"
#include "LSH.h"
//#include <ppl.h>
#include <fstream>
#include <string>
#include <ctime>
#include <omp.h>
#include <chrono>
#pragma once
//#include <Windows.h>
//using namespace concurrency;
using namespace std;

/* Author: Anshumali Shrivastava
*  COPYRIGHT PROTECTION
*  Free for research use.
*  For commercial use, contact:  RICE UNIVERSITY INVENTION & PATENT or the Author.
*/

int Bucket::_size = 64;
int LSH::_rangePow = 24;
int DensifiedMinHash::rangePow = 24;//2^24 is the _range
int K;
int L;
int ngrams = 3;
int chunk;
string inputCSVname;
string outputFile;
int MinHashChunkSize = 32;
int LSH::_thresh = 2;

string trim(string& str)
{
	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

void parseconfig(string filename)
{
	string * arguments = new string[5];
	std::ifstream file(filename);
        if(!file)
        {
           cout<<"Error Config file not found: Given Filename "<< filename << endl;
         }
	std::string str;
//	int linenum = 0;
//	int offset = 0;
	while (getline(file, str))
	{ 
		//if (str[0] == '#')
		//	continue;
                //cout<<str<< endl;
		if (str == "")
			continue;

		std::size_t found = str.find("#");
		if (found != std::string::npos)
			continue;
		
		if (trim(str).length() < 3)
			continue;
		
		
		int index = str.find_first_of("=");
		string first = str.substr(0, index);
		string second = str.substr(index + 1, str.length());



		if (trim(first) == "K")
		{
			K = atoi(trim(second).c_str());
		}
		else if (trim(first) == "L")
		{
			L = atoi(trim(second).c_str());
		}
		else if (trim(first) == "shingles")
		{
			ngrams = atoi(trim(second).c_str());
		}
		else if (trim(first) == "Chunk")
		{
			chunk = atoi(trim(second).c_str());
		}
		else if (trim(first) == "Input")
		{
			inputCSVname = trim(second);

		}
		else if (trim(first) == "Output")
		{
			outputFile = trim(second);
		}
		else if (trim(first) == "BucketSize")
		{
			Bucket::_size = atoi(trim(second).c_str());
		}
		else if (trim(first) == "MinHashChunkSize")
		{
			MinHashChunkSize = atoi(trim(second).c_str());
		}
		else if (trim(first) == "Thresh")
		{
			LSH::_thresh = atoi(trim(second).c_str());
		}
		else if (trim(first) == "RangePow")
		{
			LSH::_rangePow = atoi(trim(second).c_str());
			if (LSH::_rangePow >= 32)
				cout << "Range of Hash Values Too Big" << endl;
		}
		else
		{
			cout << "Error Parsing conf File at Line" << endl;
			cout << str << endl;
		}
	}
}

void add(string records[],int size,int offset, LSH *_Algo, DensifiedMinHash *minhasher)
{
	
    #pragma omp parallel for
	for (int i = 0; i < size; i++)
	{  
		//cout << i << endl;
		int * hashes = minhasher->getHash(records[i], ngrams);
		_Algo->add(hashes, i+offset+1);
	}
}


void retrieveAndWrite(string records[], int size, int offset, ofstream &myfile, LSH *_Algo, DensifiedMinHash *minhasher)
{
	int **retarray = new int*[size];
// #pragma omp parallel for

auto begin = chrono::high_resolution_clock::now();    
    int x;
	for (int i = 0; i < size; i++)
	{
		int id = offset + i +1;
		int *hashes = minhasher->getHash(records[i], ngrams);
		retarray[i] = _Algo->retrieve(hashes);
		retarray[i][1] = id;


		//retarray[i][0] = 100;

	}
	    auto end = chrono::high_resolution_clock::now();    
    auto dur = end - begin;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    cout << ms << endl;

	//now write sequentially
//#pragma omp serial
	for (int i = 0; i < size; i++)
	{
		for (int j = 2; j < retarray[i][0] + 1; j++)
		{
			if (retarray[i][j] == 0)
				break;
			if (retarray[i][j] <= retarray[i][1])
				continue; 
			else
			{
//#pragma omp critical
				//myfile << ": "<< retarray[i][1] << " : " << records[retarray[i][1]-1] << "   <---->    " << " : " << retarray[i][j] << " : " << records[retarray[i][j]-1] << endl;
				myfile << retarray[i][1] << " " << retarray[i][j] << endl;

			}
		}
	}
}

void RecordLinkage(int argc,char *arg[])
{ 

	//outputFile = "C:/Users/iitkg/Google Drive/RecordLinkage/sampleout";

	//int K = 2;// atoi(arg[2]);
	//int L = 100;// atoi(arg[3]);

	//
	//if (argc< 5)
	//	chunk = 50000;
	//else
	//	chunk = atoi(arg[4]);

	//parseconfig("C:/RecordLinkage/DataAndFiles/C++/Config.txt");
	cout << arg[1] << endl;
	parseconfig(arg[1]);
	
	//inputCSVname = "/Users/Beidi/Documents/fa2016/research/linkage/RecordLinkagePackage/C++Codes/rldata500p.csv";

	LSH *_Algo = new LSH(K, L);
	
        DensifiedMinHash::rangePow = LSH::_rangePow;

	DensifiedMinHash *minhasher = new DensifiedMinHash(K*L, MinHashChunkSize);
	

	string *records = new string[chunk];
	if (inputCSVname[inputCSVname.size()-1]==13){
		inputCSVname = inputCSVname.substr(0, inputCSVname.size()-1);
	}
	if (outputFile[outputFile.size()-1]==13){
		outputFile = outputFile.substr(0, outputFile.size()-1);
	}	

    std::fstream file(inputCSVname);
    if(!file.is_open())
    {
       cout<<"Error inputcsvfile not found: Given Filename "<< inputCSVname << endl;
    }  

	//Debugging	
	// string kk = "/Users/Beidi/Documents/fa2016/research/linkage/RecordLinkagePackage/C++Codes/rldata500p.csv";
	// if (trim(kk).compare(inputCSVname)!=0){
	// 		cout<<trim(kk)<<endl;
	// 		cout.flush();
	// 		cout<<inputCSVname<<endl;
	// 		cout.flush();
	// 		int i = 0;
	// 		kk = trim(kk);
	// 		for( i =0; i <kk.size();i++){
	// 			if(kk[i]!=inputCSVname[i]){
	// 				cout<<"**"<<(int)kk[i]<<"**"<<endl;
	// 			}
	// 		}
	// 		cout<<i<<endl;
	// 		cout<<(int)inputCSVname[i]<<endl;
	// 		cout<<(int)'\r'<<endl;
	// 		cout<< trim(kk).size() << " "<<inputCSVname.size()<<endl;
	// 	    cout<<trim(kk).compare(inputCSVname)<< endl;

	// }
	// std::ifstream file4(trim(kk));
	//         if(file4.is_open())
	//         {
	//            cout<<"inputcsvfile can be open "<< kk << endl;
	//         }  
      
      
	std::string str;
	getline(file, str); //remove header
	cout << str << endl;
	int linenum = 0;
	int offset = 0;
	while (getline(file, str))
	{
		records[linenum] = str;
		cout << str << endl;
		linenum++;
		if (linenum %chunk == 0)
		{
			//int offset = linenum / chunk -1;
			add(records, chunk, offset, _Algo,minhasher);
			offset += chunk;
			records = new string[chunk];
			linenum = 0;
		}
	}

	if (linenum != 0)
	{
		add(records, linenum, offset, _Algo,minhasher);
		//offset += chunk;
		//records = new string[chunk];

	}
	file.close();


	ofstream myfile(outputFile);
	//myfile << ": " << "rec1ID" << " : " << "record" << "   <---->    " << " : " << "rec2ID" << " : " << "record" << endl;
	myfile << "rec1" << " " << "rec2" << endl;

	std::ifstream file2(inputCSVname);
	getline(file2, str); //remove header
	linenum = 0;
	offset = 0;
	while (getline(file2, str))
	{
		records[linenum] = str;
		linenum++;
		if (linenum %chunk == 0)
		{
			//int offset = linenum / chunk -1;
			retrieveAndWrite(records, chunk, offset, myfile, _Algo, minhasher);
			offset += chunk;
			records = new string[chunk];
			linenum = 0;
		}
	}

	if (linenum != 0)
	{
		retrieveAndWrite(records, linenum, offset, myfile, _Algo, minhasher);
		//offset += chunk;
		//records = new string[chunk];

	}
	file2.close();
	myfile.flush();
	myfile.close();

}

int main(int argc, char* argv[])
{
	//cout << RAND_MAX << endl;
	std::time_t start, end;
	long delta = 0;
	start = std::time(NULL);


	RecordLinkage(argc, argv);

	end = std::time(NULL);
	delta = end - start;
	cout << "Takes Time: " << delta << " seconds." << endl;
	//RecordLinkage(argc, argv);
	/*
	string first = "Anshumali Shrivastava";
	string three = "Rand Shrivastava";
	string two = "Rand Shrivastava";

	HashFunction *minhasher = new DensifiedMinHash(10, 16);
	cout << minhasher->getHash(first, 3)[3] << " " << minhasher->getHash(first, 3)[1] << endl;
	cout << minhasher->getHash(two, 3)[3] << " " << minhasher->getHash(two, 3)[1] << endl;


	LSH *_Algo = new LSH(5, 2);
	_Algo->add(minhasher->getHash(first, 3), 1);
	_Algo->add(minhasher->getHash(two, 3), 2);
	_Algo->add(minhasher->getHash(three, 3), 3);

	int *ret = _Algo->retrieve(minhasher->getHash(two, 3));
	cout << ret[0] << " " << ret[1] << " " << ret[2];*/

	//cin.get();
}
