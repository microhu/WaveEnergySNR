#include "Wave.h"
#include <fstream>
#include <iostream>

using namespace std;

Wave::Wave()
{
	 numChannel=-1;
	 sampleRate=-1;
	 bitsPerSample = -1;
	 sampleNum = -1;
	 duration = -1;
}


Wave::~Wave()
{
	
}


vector<float> Wave::chopWavData(int startIndex, int endIndex)
{
	vector<float> chopedData;
	if (startIndex < 0) startIndex = 0;
	if (startIndex < 0 || endIndex <= startIndex || endIndex > sampleNum)
	{
		startIndex = 0;
		endIndex = sampleNum;
		cout << "input start or end index is illegale, will use the whole utterances as default" << endl;
	}
	for (int i = startIndex; i < endIndex; i++)
	{
		chopedData.push_back(sampData[i]);
	}
	return chopedData;
}
void Wave::Reader(const char* wavpath)
{
	try{
		ifstream iWave;
		iWave.open(wavpath, ios::binary);
		int tempK;
		char temp[5];
		temp[4] = '\0';

		iWave.read(temp, 4);
		if (strcmp(temp, "RIFF"))
		{
			throw  "wave format error, not RIFF";
		}
		int chunkSize = ReadUint32(iWave);
		iWave.read(temp, 4);
		if (strcmp(temp, "WAVE"))
		{
			throw "wave format error, not WAVE";
		}
		iWave.read(temp, 4);
		if (strcmp(temp, "fmt "))
		{
			throw "wave format error, not fmt";
		}
		int subChunk1Size = ReadUint32(iWave);
		short audioFormat = ReadUint16(iWave);
		if (audioFormat != 1)
		{
			throw "wave format error, not pcm";
		}
		numChannel = ReadUint16(iWave);
		if (numChannel != 1)
		{
			throw "Only single channel is supported yet";

		}
		sampleRate = ReadUint32(iWave);
		int byteRate = ReadUint32(iWave);
		int blockAlign = ReadUint16(iWave);
		bitsPerSample = ReadUint16(iWave);

		iWave.read(temp, 4);
		
		while (strcmp(temp, "data")) // skip other chunks
		{
			int bytes = ReadUint32(iWave);
			iWave.seekg(bytes,ios::cur);
		    iWave.read(temp, 4);
		}

		int subChunk2Size = ReadUint32(iWave);
		sampleNum = subChunk2Size / blockAlign; // sample points
		duration = sampleNum / (sampleRate / 1000); // ms
		streampos dataBeginPos = iWave.tellg();

		iWave.seekg(0, ios::end);
		int RemainSize = iWave.tellg() - dataBeginPos;
		if (RemainSize < subChunk2Size)
		{

			throw "wave format error";
		}
		iWave.seekg(dataBeginPos, ios::beg);

		std::vector<char> DataChunk(subChunk2Size);
		char *dataPtr = &DataChunk[0];

		iWave.read(dataPtr, subChunk2Size);

		for (int i = 0; i < sampleNum; i++)
		{
			switch (bitsPerSample)
			{
			case 8:
				sampData.push_back(*dataPtr);
				dataPtr++;
				break;
			case 16:
				tempK = *reinterpret_cast<short*>(dataPtr);
				sampData.push_back(tempK);
				dataPtr += 2;
				break;
			case 32:
				tempK = *reinterpret_cast<int*>(dataPtr);
				sampData.push_back(tempK);
				dataPtr += 4;
				break;
			default:
				break;
			}
		}
		iWave.close();
	}
	catch (exception &e)
	{
		cout << "error in read file: " << wavpath << ", skip this file"<<endl;
	}
}
int Wave::ReadUint32(std::istream &is) {
	union {
		char result[4];
		int ans;
	} u;
	is.read(u.result, 4);
	return u.ans;
}


short Wave::ReadUint16(std::istream &is) {
	union {
		char result[2];
		short ans;
	} u;
	is.read(u.result, 2);
	return u.ans;
}


vector<float>Wave::waveEnergyPerWindow(vector<float> sampleData, int winLen, int shift, bool DBFlag, int startPoint, int endPoint)
{
	std::vector<float> energyList;
	int WinNum = (endPoint - startPoint - winLen+shift) / shift;
	for (int i = 0; i < WinNum; i++)
	{
		long startSamplePos = startPoint + i *shift; // startpomt
		long endSamplePos = startPoint + i*shift + winLen; // endpoint
		_ASSERT(endSamplePos <= sampleData.size());

		//std::cout << meanvalue << std::endl;
		float WinEnergy = 0;
		for (long pos = startSamplePos; pos < endSamplePos; pos++)
		{
			WinEnergy += (sampleData[pos]) * (sampleData[pos]) / (endSamplePos - startSamplePos);
		}
		if (DBFlag)
			energyList.push_back(10 * log10(fmaxf(1e-10, WinEnergy))); //db
		else
			energyList.push_back(WinEnergy);
	}

	return energyList;
}

std::vector<float> Wave::AverageEnergyWithinWindow(int winLen, int shift, bool DBFlag, int startPoint, int endPoint)
{
	startPoint = startPoint >= 0 ? startPoint : 0;
	endPoint = (endPoint < startPoint || endPoint>sampleNum)? sampleNum : endPoint;

	return waveEnergyPerWindow(sampData, winLen, shift, DBFlag, startPoint, endPoint);
}

long Wave::numberOfSamples()
{
	return sampData.size();
}
void Wave::writeToMonoWavFile(const vector<float>& data, const string &wavFile)
{
	int sampleRate = 16000;
	short nchannel = 1;
	int nsample = data.size();
	short bitsPerSample = 16;
	int subChunk1Size = 16;
	short audioFormat = 1;
	int subChunk2Size = nsample*nchannel*bitsPerSample / 8;
	int chunkSize = 36 + subChunk2Size;
	int byteRate = sampleRate*nchannel*bitsPerSample / 8;
	short blockAlign = nchannel*bitsPerSample / 8;

	ofstream wavWriter(wavFile,fstream::binary);
	if (wavWriter.is_open())
	{
		wavWriter.write((char*)("RIFF"), 4);
		wavWriter.write((char*)&chunkSize, 4);
		wavWriter.write((char*)("WAVE"), 4);

		wavWriter.write((char*)("fmt "), 4);
		wavWriter.write((char*)&subChunk1Size, 4);
		wavWriter.write((char*)&audioFormat, 2);
		wavWriter.write((char*)&nchannel, 2);
		wavWriter.write((char*)&sampleRate, 4);
		wavWriter.write((char*)&byteRate, 4);
		wavWriter.write((char*)&blockAlign, 2);
		wavWriter.write((char*)&bitsPerSample, 2);
		wavWriter.write((char*)("data"), 4);
		wavWriter.write((char*)&subChunk2Size, 4);
		for (int i = 0; i < data.size(); i++)
		{
			short tempV = data[i];
			wavWriter.write((char*)&(tempV), 2);
		}
			
		wavWriter.close();
	}
	else
	{
		
		string message = "unable to open file: " + wavFile;
		cout << message.c_str() << endl;
		throw message.c_str();
		
	}
}