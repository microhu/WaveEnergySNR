#pragma once
using namespace std;
#include<vector>
class gmDist
{
public:
	gmDist();
	gmDist(int, float[], float[], float[]);
	~gmDist();
	void GaussianClusteringFlatStart(const int &N_class, const vector<float> & sampleDa, const double &epsion);
	void GaussianClustering(std::vector<float> trainData, double epsion);
private:
	const static double LZERO;
	const static double ZERO;
	const static double minLogExp;
	const static double LSMALL;
	const static double M_PI;

    std::vector<float> gmmOccupationCalculate(float feat, double&);
	double GaummsianProb(int centid, float samplePoint);
	double LAdd(double A, double B);
	
	double EMUpdateGMMOnce(std::vector<float> trainData);
public:
	int N_class;
	std::vector<float> priors;
	std::vector<float> Means;
	std::vector<float> Variances;
};

