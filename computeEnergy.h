#ifndef computeEnergy_h
#define computeEnergy_h
#include "integrator.h"
#include "legendre.h"
#include <algorithm>
using namespace std;

extern int proton_number;
extern int neutron_number;
extern double hbarc,ms,mv,mp,mg,gs,gv,gp,gg,lambdas,lambdav, lambda, ks,ka;

pair<double, double> compute_energy2(vector<eig2> &occp,vector<eig2> &occn,vector<vector<double>> &Phi,
                      vector<vector<double>> &W, vector<vector<double>> &B, vector<vector<double>> &A,
                      vector<vector<double>> &dens, vector<vector<double>> &denv, vector<vector<double>> &den3,
                      vector<vector<double>> &denp) {
    double bindEnergySum = 0.0;
    for (int i = 0; i < occn.size(); i++) {
        bindEnergySum += occn[i].solution.eigen_values;
    }
    for (int i = 0; i < occp.size(); i++) {
        bindEnergySum += occp[i].solution.eigen_values;
    }
    
    vector<double> inte(N, 0.0);
    for (int i = 0; i < N; i++) {
        for (int l = 0; l < max_L; l++) {
            inte[i] += (-Phi[l][i] * dens[l][i]) / (2 * l + 1.0);
            inte[i] += (W[l][i] * denv[l][i]) / (2 * l + 1.0);
            inte[i] += (0.5 * B[l][i] * den3[l][i]) / (2 * l + 1.0);
            inte[i] += (A[l][i] * denp[l][i]) / (2 * l + 1.0);
        }
        inte[i] *= pow(fx[i], 2);
        inte[i] *= (2 * PI);
    }
    double ecm=3.0*41.0*pow((proton_number+neutron_number),-4.0/3)/4.0;
    double ret = bindEnergySum - my_spline(inte, fx, my_tolerance).integral();
    double be = ret / (proton_number + neutron_number) - ecm;
//    cout << "B/A is " << be << endl;
    return make_pair(be, ret);
}


vector<double> computeRadius(vector<vector<double>> &denv, vector<vector<double>> &denp) {
    vector<vector<double>> denn(denv.size(), vector<double>(denv[0].size(), 0.0));
    for (int i = 0; i < denv.size(); i++) {
        for (int j = 0; j < denv[0].size(); j++) {
            denn[i][j] = denv[i][j] - denp[i][j];
        }
    }
    vector<double> inte(N, 0.0);
    double rnn, rnp;
    for (int i = 0; i < N; i++) {
        inte[i] = pow(fx[i], 4) * denn[0][i]; 
    }
    rnn = 4 * PI * my_spline(inte, fx, my_tolerance).integral() / neutron_number;
    for (int i = 0; i < N; i++) {
        inte[i] = pow(fx[i], 4) * denp[0][i];
    }
    rnp = 4 * PI * my_spline(inte, fx, my_tolerance).integral() / proton_number;
    return {sqrt(rnp), sqrt(rnn), - sqrt(rnp) + sqrt(rnn)};
}



















#endif
