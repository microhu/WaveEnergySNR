#include "gmDist.h"
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;

gmDist::gmDist()
{
}

gmDist::gmDist(int n, float occps[], float mean[], float vars[])
{
	N_class = n;
	priors.resize(N_class);
	Means.resize(N_class);
	Variances.resize(N_class);
	for (int i = 0; i < N_class; i++)
	{
		priors[i] = occps[i];
		Means[i] = mean[i];
		Variances[i] = vars[i];
	}
}
gmDist::~gmDist()
{
}
 const double gmDist::LZERO = -pow(10, 10);
 const double gmDist::ZERO = pow(10, -10);
 const double gmDist::minLogExp = -log(-gmDist::LZERO);
 const double gmDist::LSMALL = -0.5*pow(10, 10);
 const double gmDist::M_PI = 3.14159265358979323846;

 std::double_t gmDist::GaummsianProb(int centid, float feat)
 {
	 _ASSERT(centid < N_class);
	 double prob = log(2*M_PI*Variances[centid]); // gconst part
	 prob += (feat - Means[centid])*(feat - Means[centid]) / fmax(0.000000001, Variances[centid]);
	 prob = -0.5f*prob;
	 return prob; // log scale value
 }

 std::double_t gmDist::LAdd(double A, double B)
 {
	 double temp = 0;
	 double diff = 0;
	 double C = 0;
	 if (A < B)
	 {
		 temp = A;
		 A = B;
		 B = temp;
	 }
	 diff = B - A;
	 if (diff < minLogExp)
	 {
		 return (A < LSMALL) ? LZERO : A;
	 }
	 else
	 {
		 C = exp(diff);
		 return A + log(1.0 + C);
	 }
 }

 std::vector<float> gmDist::gmmOccupationCalculate(float samplePoint, double& totalLikelihood)
{
	 vector<float> occupations(N_class);
	 vector<double> likelihoods(N_class,0);
	 totalLikelihood = 0;
	 double logsumWeight = LZERO;
	for (int i = 0; i < N_class; i++)
	{
		occupations[i] = log(priors[i]) + GaummsianProb(i, samplePoint);
		likelihoods[i] = occupations[i];
		logsumWeight = LAdd(logsumWeight, occupations[i]);
	}
	for (int i = 0; i < N_class; i++) occupations[i] = exp(occupations[i] - logsumWeight);
	for (int i = 0; i < N_class; i++) totalLikelihood += occupations[i] * likelihoods[i];
	return occupations;
}

 std::double_t gmDist::EMUpdateGMMOnce(std::vector<float> trainData)
 {
	 double logLikelihood = LZERO;

	 std::vector<float> r(N_class);
	 std::vector<float> rX(N_class);
	 std::vector<float> rXX(N_class);

	 int N = trainData.size();

	 for (int i = 0; i < N; i++)
	 {
		 double tempLoglikelihood = 0;
		 std::vector<float> occup = gmmOccupationCalculate(trainData[i], tempLoglikelihood);
		 logLikelihood=LAdd(logLikelihood, tempLoglikelihood);
		 for (int j = 0; j < N_class; j++)
		 {
			 r[j] += occup[j];
			 rX[j] += occup[j] * trainData[i];
			 rXX[j] += occup[j] * trainData[i] * trainData[i];
		 }
	 }

	 // update model
	 for (int i = 0; i < N_class; i++)
	 {

		 Means[i] = rX[i] / r[i];
		 Variances[i] = fmax(0.00000001, rXX[i] / r[i] - Means[i] * Means[i]); // variance not updated
		 priors[i] = r[i] / N;
	 }

	 return logLikelihood;
 }
 void gmDist::GaussianClustering(std::vector<float> trainData, double epsion)
 {
	 double logLikelihood0 = -100;
	 double loglikelihoodUpdated = EMUpdateGMMOnce(trainData);
	 int iterationNum = 0;
	 while (abs((loglikelihoodUpdated - logLikelihood0)/logLikelihood0) > epsion)
	 {
		 logLikelihood0 = loglikelihoodUpdated;
		 loglikelihoodUpdated = EMUpdateGMMOnce(trainData);
		 iterationNum++;
	//	 std::cout <<"LogLikelihood for Step"<< iterationNum << ":\t" << logLikelihood0<<endl;
	 }
	// std::cout << "LogLikelihood for Step" << iterationNum << ":\t" << logLikelihood0 << endl;
 }

 void gmDist::GaussianClusteringFlatStart(const int &nClass, const vector<float> &trainData, const double &epsion)
 {

	 N_class = nClass;
	 priors.resize(N_class);
	 Means.resize(N_class);
	 Variances.resize(N_class);
	 if (trainData.empty())
	 {
		 cout << "error: no training data is avaiable" << endl;
		 return;
	 }
	 int N = trainData.size();
	 float maxValue = *std::max_element(std::begin(trainData), std::end(trainData));
	 float minValue = *std::min_element(std::begin(trainData), std::end(trainData));
	 float mean0 = 0;
	 float var0 = 0;
	 for (int i = 0; i < N; i++)
	 {
		 mean0 += trainData[i] / N;
		 var0 += trainData[i] * trainData[i] / N;
	 }
	 var0 -= mean0*mean0;

	 Means[0] = mean0 - sqrt(var0);
	 Means[1] = mean0 + sqrt(var0);

	 for (int i = 0; i < N_class; i++)
	 {
		 Means[i] = mean0 - sqrt(var0) + 2 * i*sqrt(var0) / (N_class - 1);
		 priors[i] = 1.0f/N_class;
		 Variances[i] = var0;
	 }
	 GaussianClustering(trainData, epsion);
 }