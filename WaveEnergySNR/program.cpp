#include<iostream>
#include<fstream>
#include<string>
#include<sstream>

#include<algorithm>
#include<map>
#include "Wave.h"
#include "gmDist.h"

#define NumClass 2
using namespace std;

void printOutDataPoints(char* outlog, std::vector<float> points);
void PrintOutResults(char* outlog, int N, float maxvalue, float minvalue, gmDist igmm);
void WaveEnergyAnlysis(char* wavepath, char*outlog);
map<string, string>  readIdKeyVaulePair(char* idwavFile);
void localWindowSnrEstimation(const string &wavePath, const float windowLengthInMs, const string &outLog);
int main(int argc, char* argv[])
{

	string wavPath = "D:\\code\\Baum-Welch\\ViterbiDecoder\\data\\id1_0_imp_M0.91R0.91.IMP_snr_15.wav";
	string logFilePath = "D:\\code\\Baum-Welch\\ViterbiDecoder\\data\\id1_0_imp_M0.91R0.91.IMP_snr_15.snr";
	localWindowSnrEstimation(wavPath, 500, logFilePath);


	if (argc < 3)
	{
		cout << "usage: exe --snr idwavpath logdir"<<endl;
		cout << "usage: exe --energy wavefile logfile DBFlag" << endl;
	}

	string command = argv[1];

	if (!strcmp(command.c_str(), "--snr"))
	{
		string idwavFile = argv[2];
		string logdir = argv[3];


		map<string, string> IdWavMap = readIdKeyVaulePair(&idwavFile[0]);
		for (std::map<string, string>::iterator iter = IdWavMap.begin(); iter != IdWavMap.end(); iter++)
		{
			string logpath = logdir + "\\" + iter->first + ".snr.log";
			WaveEnergyAnlysis(&iter->second[0], &logpath[0]);
		}
	}
	else if (!strcmp(command.c_str(), "--energy"))
	{
		
		string wavfile = argv[2];
		string logfile = argv[3];
		bool dbformat = atoi(argv[4]) > 0 ? true : false;
		Wave iwave;
		iwave.Reader(&wavfile[0]);
		std::vector<float> Energy = iwave.AverageEnergyWithinWindow(25*iwave.getSampleRate()/1000, 10*iwave.getSampleRate()/1000, dbformat,0,-1);
		printOutDataPoints(&logfile[0], Energy);
	}
	else
	{

	}



}

map<string, string>  readIdKeyVaulePair(char* idwavFile)
{
	map<string, string> idmap;
	string line,key,value;
	ifstream myfile(idwavFile);
	if (myfile.is_open())
	{
		while (getline(myfile, line))
		{
			istringstream iss(line);
			iss >> key;
			iss >> value;
			idmap.insert(std::pair<string, string>(key, value));
		}
		myfile.close();
		
	}
	else
	{
		cout << "Unable to open file" << endl;
	}
	return idmap;
}
void WaveEnergyAnlysis(char* wavepath, char*outlog)
{
	Wave iwave;
	iwave.Reader(wavepath);
	std::vector<float> Energy = iwave.AverageEnergyWithinWindow(25 * iwave.getSampleRate() / 1000, 10 * iwave.getSampleRate() / 1000, true, 0, -1);
	int N = Energy.size();
	float maxValue = *std::max_element(std::begin(Energy), std::end(Energy));
	float minValue = *std::min_element(std::begin(Energy), std::end(Energy));
	float mean0 = 0;
	float var0 = 0;
	for (int i = 0; i < N; i++)
	{
		mean0 += Energy[i] / N;
		var0 += Energy[i] * Energy[i] / N;
	}
	var0 -= mean0*mean0;

	float initialMean[NumClass];
	float initialPrior[NumClass];
	float initialVar[NumClass];

	initialMean[0] = mean0 - sqrt(var0);
	initialMean[1] = mean0 + sqrt(var0);
	for (int i = 0; i < NumClass; i++)
	{
		initialPrior[i] = 0.5;
		initialVar[i] = var0;
	}

	gmDist igmm(NumClass, initialPrior, initialMean, initialVar);
	igmm.GaussianClustering(Energy, 0.0001);

	PrintOutResults(outlog, N, maxValue, minValue, igmm);
	//printOutDataPoints(energylog, Energy);
}
void printOutDataPoints(char* outlog, std::vector<float> points)
{
	ofstream myfile(outlog);
	if (myfile.is_open())
	{
		for (int i = 0; i < points.size(); i++)
			myfile << points[i] << endl;
		myfile.close();
	}
	else
	{
		cout << "unable to open file" << endl;
	}
	

}
void PrintOutResults(char* outlog, int N, float maxvalue, float minvalue, gmDist igmm)
{
	ofstream myfile(outlog);
	if(myfile.is_open())
	{
		
		myfile << "Num Sample:\t" << N << endl;
		myfile << "Max value:\t" << maxvalue << endl;
		myfile << "Min value:\t" << minvalue << endl;

		myfile << "Gaussian prior:\t " << igmm.priors[0]<<"\t"<<igmm.priors[1] << endl;
		myfile << "Gaussian means:\t " << igmm.Means[0] << "\t" << igmm.Means[1] << endl;
		myfile << "Gaussian variance:\t " << igmm.Variances[0] << "\t" << igmm.Variances[1] << endl;
		float noiseMean = fmin(igmm.Means[0], igmm.Means[1]);
		float signalMean = fmax(igmm.Means[0], igmm.Means[1]);

		myfile << "SNR:\t" << signalMean-noiseMean<< endl;
		myfile << "PSNR:\t" << maxvalue - noiseMean << endl;
		myfile.close();
	}
	else
	{
		cout << "Unable to open file" << endl;
	}

}

void localWindowSnrEstimation(const string &wavePath,  const float windowLengthInMs, const string &outLog)
{
	int frameWinlenInMs = 25; // ms
	int frameShiftLenInMs = 10; // ms
	vector<string> resultLines;
	Wave iwave;
	iwave.Reader(wavePath.c_str());
	long nSamples=iwave.numberOfSamples();
	int sampleRate = iwave.getSampleRate();
	float durationInMs = (nSamples / (float)sampleRate) * 1000;
	int windowN = durationInMs / windowLengthInMs;;
	int nSamplesInWindow = windowLengthInMs*sampleRate / 1000;

//	vector<float> energyDataInDB = iwave.AverageEnergyWithinWindow(25, 10, true);

	for (int k = 0; k < windowN; k++)
	{
		int startPoint = k*nSamplesInWindow;
		int endPoint = (k + 1)*nSamplesInWindow + frameWinlenInMs * iwave.getSampleRate() / 1000;
	//	vector<float> chopedData=iwave.chopWavData(startPoint, endPoint);
	//	for (int i = 0; i < chopedData.size(); i++) chopedData[i] = fmax( 20 * log10(abs(chopedData[i])),0); // converted to energy power amptitude

		vector<float> chopedData = iwave.AverageEnergyWithinWindow(frameWinlenInMs * iwave.getSampleRate() / 1000, frameShiftLenInMs* iwave.getSampleRate() / 1000, true, startPoint, endPoint);
		gmDist igmm;
		igmm.GaussianClusteringFlatStart(2, chopedData, 0.01);
		float minMean = fmin(igmm.Means[0], igmm.Means[1]);
		float maxMean = fmax(igmm.Means[0], igmm.Means[1]);
		float peakV = *std::max_element(std::begin(chopedData), std::end(chopedData));
		float snr = maxMean - minMean;// 10 * log10(maxMean) - 10 * log10(minMean);
		float psnr = peakV - minMean;// 10 * log10(peakV) - 10 * log10(minMean);
		string line = to_string(k*windowLengthInMs) + "  " + to_string((k + 1)*windowLengthInMs) + " " + to_string(snr) + "  " + to_string(psnr) +" "+to_string(igmm.Means[0]) + " "+to_string(igmm.Means[1]);
		cout << line << endl;
		resultLines.push_back(line);
	}
}