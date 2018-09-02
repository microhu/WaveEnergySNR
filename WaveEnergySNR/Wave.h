#pragma once
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;
class Wave
{

public:
	Wave();
	~Wave();	
	
	void Reader(const char* wavpath);
	std::vector<float> AverageEnergyWithinWindow(int winlen, int shift, bool logFlag=true, int startPoint=0, int endPoint=-1);
	long  numberOfSamples();
	int getSampleRate(){ return sampleRate; };
	vector<float> chopWavData(int startIndex, int endIndex);
	static void  writeToMonoWavFile(const vector<float> &data, const string &wavFile);
	static vector<float> waveEnergyPerWindow(vector<float> sampleData, int winLen, int shift, bool DBFlag, int startPoint, int endPoint);
private:
	int numChannel;
	int sampleRate;
	int bitsPerSample;
	long sampleNum;
	float duration;
	std::vector<float> sampData;
	int ReadUint32(std::istream &is);
	short ReadUint16(std::istream &is);


};

