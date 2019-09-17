/*
 The added potential right now is r Y10 and in units Mev.
*/
#include <chrono>
#include "vector_compute.h"
#include "generate_state.h"
#include "dirac_oscillator.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <fstream>
#include "integrator.h"
#include "generate_matrix.h"
#include "simps.h"
#include "symm.h"
#include "Solution.h"
#include "Density.h"
#include "Green-method.h"

#include "utility.h"
#include "computeEnergy.h"
#include <algorithm>
#include <iomanip>
using namespace std;
#define MAX_KAPPA 7
/*--------------------set up global varialbes-----------------------------*/
/*constants*/
const double PI=atan(1.0)*4;
//string parameters = "/Users/junjieyang/Dropbox/project/Deform_c_new/parameter.txt";  // the file contains the model parameters
string parameters = "parameter.txt";
double my_tolerance=1e-4;   //tolerance of the integrator
/*model parameters, default values are linear*/
double hbarc=197.326;
double ms=520.0/hbarc;
double mv=782.5/hbarc;
double mp=763.0/hbarc;
double mg=0.00000001/hbarc;
double gs=109.63;
double gv=190.43;
double gp=65.23;
double gg=4*PI/137.0;
double lambdas=0.0;
double lambdav=0.0;
double lambda=0.0;
double ks=0.0000;
double ka=0.0/hbarc;


/* Computation constants */
double update_ratio = 1.0 / 7;    //this is a parameter that can be adjusted
double b=2.4,rmin=0.0,rmax=20.0;
int N=801;
int max_L = 1;   /*Max value of L CHANNEL*/   
int Potential_channel = 1;     /*The potential I want to add*/
double Deformation_parameter = 0.00;
double h=(rmax-rmin)/(N-1);       //grid size
int proton_number, neutron_number;
vector<double> start ={450.0,400.0,5.0,0.0}; //initial value of Phi W B A in the wood-saxon form, units in Mev
vector<double> fx;                //x axis
vector<double> empty(N, 0.0);             //empty vector, will be initialized later
unordered_map<string, vector<vector<double>> > States_map;   //store the infos about the wavefunctions when given states
unordered_map<string, double> Energy_map;                    //store the energies for every state
int max_k; //max kappa when I want to construct the basis 
int itenum;
int nodes;
/*---------------------------------finishing global varialbes-------------------------------------*/
#include "preprocess.h"
#include "effective_density.h"


/*body of the function*/
int main(int argc, char ** argv){
    //Input part and the parameters
    //the arguments contains : parameter number, proton number, neutron number, outputfilename
    string parameterName = argv[1];
    string outputfileName;
    cout << "arguments is " << parameterName  << endl;
    //set the parameter name
    parameters = "parameters/" + parameterName + ".txt"; 
    // parameters = "parameters/" + parameterName + ".txt";
    preprocessing();                    //Prepare the states HashMap
    itenum = 69;
    //update the proton number and neutron number as well as the output file name
    proton_number = atoi(argv[2]);
    neutron_number = atoi(argv[3]);
    outputfileName = argv[4];
    max_k = atoi(argv[5]);
    int update_num = atoi(argv[6]);
    update_ratio = 1.0 / update_num;
    string nuclei_name = "";
    for (int i = 0; i < outputfileName.size(); i++) {
    	if (outputfileName[i] == ' ') {
    		for (int j = i + 1; j < outputfileName.size(); j++) {
    			nuclei_name += outputfileName[j];
    		}
    		break; 
    	}
    }
    //print the arguments just in case
    cout << "other arguments " << proton_number << ' ' << neutron_number << ' ' << outputfileName << ' ' 
         << max_k << endl; 
    //define the variables
    double oldmin = 0.0;      //the min of the L = 1 density channel
    chrono::steady_clock::time_point tp1 = chrono::steady_clock::now();  //current time
    unordered_map<string, vector<vector<double>>>::iterator States_ptr;  //store oscillator wavefunctions
    vector<vector<double>> Phi(max_L,vector<double>(N, 0.0));            //set up meson potentials
    vector<vector<double>> W = Phi, B = Phi, A = Phi;
    vector<vector<double>> dens = Phi, denv = Phi, denp = Phi, den3 = Phi;  //set up density
    vector<vector<double>> EFF_Phi = Phi, EFF_W = Phi, EFF_B = Phi, EFF_A = Phi;   //set up effective density
    vector<vector<double>> dens_old = Phi, denv_old = Phi, denp_old = Phi, den3_old = Phi;  //soft update density   
    vector<vector<double>> scalar_p = Phi, vector_p = Phi, scalar_n = Phi, vector_n = Phi;  //set up scalar and vector poetntial
    vector<double> Potential(N, 0.0);   //dipole potential
    vector<double> flat; //flatted matrix
    vector<State> States_m;              //the basis quantum number of a specific m
    vector<eig2> occp,occn;              //occupied states of protons and neutrons
    vector<eig2> occp_raw,occn_raw;      //raw solution of matrix
    vector<eig2> temp_solution;          //eig2 contains eig, m; eig contains eigenvalues and eigenvectors
                                         //temp solution contains the result for one matrix diag
    vector<double> allEnergies(itenum, 0.0);   //store total energy of the system
    vector<double> radius;                     //store reaidus of the system(rp, rn, rskin)
    double bindingPerParticle;                 //store binding energy per particles
    preprocessing_2(Phi, W, B, A);      //initialize the PHI W B A potential the potentials here are all in Mev
    EFF_Phi=dens; EFF_W=dens;EFF_A=dens;EFF_B=dens;  /*Set the effecitive density in KG part to be the same as
                                                      original density*/
    /*Determine the range of m, just roughly determine*/
    int min_m = -magic(max(proton_number,neutron_number));
    int max_m = -min_m;
    cout << "M range is: " << min_m << " -> " << max_m << endl;
    vector<Solution> Final_occp,Final_occn;                 //Final solution for every iteration
    //The self consistent calculations
    for(int ite=0;ite<itenum;ite++){
        chrono::steady_clock::time_point tpold = chrono::steady_clock::now();
        /*Make sure OCCs are empty*/
        occp_raw.clear();
        occn_raw.clear();
        occp.clear();
        occn.clear();
        //because this the added dipole operator contains lambda * tauz *  r * Y10
        for(int i=0; i<N; i++)    Potential[i] = 0.5 * fx[i] * Deformation_parameter * sqrt(3.0 / 4 / PI);
        /*Every iteration it updates the meson potentials,so it need to recompute the scalar and vector potentials
          those two functions are in preprocess.h
          compute the scalar and vector potential based on the meson fields
         */
        generate_potential(Phi, W, B, A, Potential, scalar_n, vector_n, Potential_channel, 0);   //neutron   preprocess.h
        generate_potential(Phi, W, B, A, Potential, scalar_p, vector_p, Potential_channel, 1);   //proton
        /* Solve the dirac equation*/
        for(int m = min_m ; m < max_m + 1 ; m+=2){      //The m here is actually 2 m, so add 2 every iteration
            States_m=generate_statesm(m, max_L, nodes);        //get all states based on m and MaxL
            vector<vector<double>> M = generate_full_matrix(scalar_n, vector_n, States_m, m); //generate the matrix for neutrons
            if (M.size()!=States_m.size()) cout<<"States Size != Matrix size"<<endl;
            /*diagnolize the matrix, add all the eigenvalues and eigenvectors into M_matrix*/
            matrix_diag diag = matrix_diag(M);
            diag.get_results();
            /*diag.results is a matrix full of eigvalues and eigenvectors.
              I need to add m to the result to form the basis*/
            temp_solution = get_temp_solution(diag.results, m);
            /*Insert all solutions for this m*/
            occn_raw.insert(occn_raw.end(), temp_solution.begin(), temp_solution.end());
            /*get the matrix for the specific m,proton*/
            M=generate_full_matrix(scalar_p, vector_p, States_m, m);
            diag=matrix_diag(M);
            diag.get_results();
            temp_solution = get_temp_solution(diag.results, m);
            occp_raw.insert(occp_raw.end(),temp_solution.begin(),temp_solution.end());
        }
        /* Form the solution for occupied state*/
        get_solution(occp_raw, occn_raw, occp, occn);            //get sorted occ state, utility.h
        //I didn't use final occp and final occn here(maybe I can delete them)
        Final_occn=get_solutions_object(occn);                   //get solution object, density.h
        Final_occp=get_solutions_object(occp);
        /* Form the densities */
        generate_density(occn,occp,dens,denv,denp,den3); //calculate all the density based on the occupied states, density.h
//  soft update the density 
//        if(ite > 0){
//            for(int i = 0; i < max_L; i++){
//                 for(int j = 0; j < N; j++){
//                     dens[i][j] = 3./4 * dens_old[i][j] + 1.0/4 * dens[i][j];
//                     denv[i][j] = 3./4 * denv_old[i][j] + 1.0/4 * denv[i][j];
//                     den3[i][j] = 3./4 * den3_old[i][j] + 1.0/4 * den3[i][j];
//                     denp[i][j] = 3./4 * denp_old[i][j] + 1.0/4 * denp[i][j];
//                 }
//             }
//         }    
//        dens_old = dens;
//        denv_old = denv;
//        den3_old = den3;
//        denp_old = denp;
        
        //output the density
        for(int i = 0; i < max_L; i++) {
            cout << "Channel=" << i << ':' << dens[i][0] << ' ' << denv[i][0] << ' ' << den3[i][0] << ' ' << denp[i][0] << endl;
        }
        
        /* find the minimum */
        int min_index = -1;
        double min_dens = 10;
        if(max_L > 1){
            for(int i =0; i < N; i ++){
                if (denv[1][i] < min_dens){
                    min_dens = denv[1][i];
                    min_index = i;
                }
            }
        }
        
        /* compute the effective density solve the klein-gordon equation update the potentials*/
        update_potential(EFF_Phi, EFF_B, EFF_A, EFF_W, Phi, W, B, A, dens, denv, den3, denp);
        for(int i = 0; i < max_L; i++) {
            cout <<"channel="<<i<<':'<< Phi[i][50] <<' '<< W[i][50]<<' '<<B[i][50]<<' '<<A[i][50]<<endl;
        }
        /*check how many occ states we found for each iteration*/
        cout<<occp.size()<<' '<<occn.size()<<endl;
        // cout << "minimum" << ' ' << min_index << ' ' << min_dens << ' ' << min_dens - oldmin << endl;
        oldmin = min_dens;
        
        /*get energy and radius*/
        allEnergies[ite] = compute_energy2(occp, occn, Phi, W, B, A, dens, denv, den3, denp);
        if (abs(allEnergies[ite]) > 2500) break;
        // cout<<"total energy = " << setprecision(9) << allEnergies[ite] << endl;
        bindingPerParticle =  compute_energy(occp, occn, Phi, W, B, A, dens, denv, den3, denp);
        cout<<"E/A="<< bindingPerParticle <<endl;
        radius = computeRadius(denv, denp);
        double rc = sqrt(radius[0] * radius[0] + 0.88 * 0.88 - neutron_number * 0.116 / proton_number);
        cout<<"rp = " << radius[0] << " rn = " << radius[1] << " skin = " << radius[2] << " rc = " << rc << endl;
        chrono::steady_clock::time_point tpnew = chrono::steady_clock::now();
        chrono::steady_clock::duration duration_in_ites = tpnew - tpold;
        cout << "Time_used_in_iteration " <<  ite << " = " <<  chrono::duration_cast<chrono::seconds>(duration_in_ites).count() << endl;
    } //end the iterations
    
    /* option part, output all the potentials and densities */
    // cout << "output all potentials and densities? choose y/n" << endl;
    string flag = "y";
    // cin >> flag;
    if (flag == "y") {
    	for(int i = 0 ;i < max_L; i++){
        	ofstream outfile;
        	ofstream outfile2;
        	outfile.open("density" + to_string(i) + ".txt");
        	outfile2.open("potential" + to_string(i) + ".txt");
        	for(int j = 0; j < N; j++){
            	outfile << fx[j] << ' ' << dens[i][j] << ' ' << denv[i][j] << ' ' << den3[i][j] << ' ' << denp[i][j] << endl;
            	outfile2 << fx[j] << ' ' << Phi[i][j] << ' ' << W[i][j] << ' ' << B[i][j] << ' ' << A[i][j] << endl;
        	}
        	outfile.close();
        	outfile2.close();
    	}
    } 
    
    //output the L = 1 channel of density
    if(max_L > 1){
        ofstream outfile;
        outfile.open("output/density" + to_string(Deformation_parameter) + ".txt");
        for(int j = 0; j < N; j++){
            outfile << fx[j] << ' ' << dens[1][j] << ' ' << denv[1][j] << ' ' << den3[1][j] << ' ' << denp[1][j] << endl;
        }
    }
    
    //output the energies over iterations
    ofstream energyfile;
    energyfile.open("output/totalenergies" + to_string(Deformation_parameter) + ".txt");
    for (int i = 0; i < itenum; i++) {
        energyfile << setprecision(9) << allEnergies[i] << endl;
    }
    energyfile.close();
    
    /* output the single state energy*/
    energyfile.open("output/singleStateEnergy" + to_string(Deformation_parameter) + ".txt");
    for(int i = 0; i < occp.size(); i ++){
        eig2 test = occp[i];
        Solution temp = Solution(test);
        temp.get_primary_state();
        energyfile << i << setprecision(9) << " energy for occp = " << temp.energy <<
             ' ' << "m = " << temp.m << " primary_state = (n, k, m, sign)" << temp.primary_state << endl;
    }
    for(int i = 0; i < occn.size(); i ++){
        eig2 test = occn[i];
        Solution temp = Solution(test);
        temp.get_primary_state();
        energyfile << i << setprecision(9) << " energy for occn = " << temp.energy << ' '
         << "m = " << temp.m << " primary_state = (n, k, m, sign)" << temp.primary_state << endl;        
    }
    energyfile.close();



    /*output the spin orbit term*/
    //proton, neutron
    Density all_states_p(occp), all_states_n(occn);
    all_states_p.spin_orbit_density();       //compute the spin-orbit for proton and neutron
    all_states_n.spin_orbit_density();
    ofstream so_term;
    so_term.open("output/so_density/" + parameterName +  nuclei_name + "spin-orbit.txt");
    for (int i = 0; i < N; i++) {
        so_term << fx[i] << " " << all_states_p.so[i] << " " << all_states_n.so[i] << endl;
    }
    so_term.close();

    ofstream outfile;
    outfile.open("output/so_density/" + parameterName +  nuclei_name + "density.txt");
    for(int j = 0; j < N; j++){
        outfile << fx[j] << ' ' << dens[0][j] << ' ' << denv[0][j] << ' ' << den3[0][j] << ' ' << denp[0][j] << endl;
    }
    outfile.close();






    //--------------------------------------------------------
    /* get time */
    chrono::steady_clock::time_point tp2 = chrono::steady_clock::now();
    chrono::steady_clock::duration d = tp2-tp1;
    cout <<"Total time used:"<<chrono::duration_cast<chrono::seconds>(d).count()<<endl;
    cout <<"maxL = "<<max_L<< endl;
    cout <<"Deformation parameter = "<< Deformation_parameter << endl;


    //output neutron skin
    ofstream testoutputfile;
    testoutputfile.open("output/" + outputfileName + "radius.txt", std::ios_base::app);
    double rc = sqrt(radius[0] * radius[0] + 0.88 * 0.88 - neutron_number * 0.116 / proton_number);
  	testoutputfile << parameterName << ' ' <<  proton_number << ' ' << neutron_number << ' ' << bindingPerParticle << ' ' << radius[0] << ' ' << radius[1] << ' ' << radius[2] << ' ' << rc << endl; 
  	return 0;
}


/* output wavefunctions */
//     for(int ii = 0; ii < occp.size(); ii ++){
//         eig2 test = occp[ii];
//         Solution temp = Solution(test);
//         temp.get_all_wavefunction();
//         cout << ii << " energy for occp = " << temp.energy << endl;
// //        mkdir("wfp" + to_string(ii));
//         string some = "mkdir " + string("testout/wfp") + to_string(ii);
//         const char * path = some.c_str();
//         system(path);
//         for(int i = 0; i < temp.wavefunctions.size(); i ++){
//             ofstream outfile;
//             outfile.open("testout/wfp" + to_string(ii) + '/' + "wavefunction" + to_string(temp.wavefunctions[i].kappa) + ".txt");
//             for(int j = 0; j < N; j++){
//                 outfile << fx[j] << ' ' << temp.wavefunctions[i].upper[j] << ' ' << temp.wavefunctions[i].lower[j] << ' ' << temp.m << ' ' << temp.energy << endl;
//             }
//             outfile.close();
//         }
//     }
// for(int ii = 0; ii < occn.size(); ii ++){
//     eig2 test = occn[ii];
//     Solution temp = Solution(test);
//     temp.get_all_wavefunction();
//     cout << ii << " energy for occn = " << temp.energy << endl;
//     string some = "mkdir " + string("testout/wfn") + to_string(ii);
//     const char * path = some.c_str();
//     system(path);
//     for(int i = 0; i < temp.wavefunctions.size(); i ++){
//         ofstream outfile;
//         outfile.open("testout/wfn" + to_string(ii) + '/' + "wavefunction" + to_string(temp.wavefunctions[i].kappa) + ".txt");
//         for(int j = 0; j < N; j++){
//             outfile << fx[j] << ' ' << temp.wavefunctions[i].upper[j] << ' ' << temp.wavefunctions[i].lower[j] << ' ' << temp.m << ' ' << temp.energy << endl;
//         }
//         outfile.close();
//     }
// }


//output density during the iterations
//         for(int i = 0 ;i < max_L; i++){
//             if((ite % 50) == 0 && i == 1){
//             ofstream outfile;
//             ofstream outfile2;
//             outfile.open("output/density" + to_string(i) + ' ' + to_string(ite) + ".dat");
//              outfile2.open("output/potential" + to_string(i) + ' ' + to_string(ite) +  ".txt");
//             for(int j = 0; j < N; j++){
//                 outfile << fx[j] << ' ' << dens[i][j] << ' ' << denv[i][j] << ' ' << den3[i][j] << ' ' << denp[i][j] << endl;
//                 // outfile2 << fx[j] << ' ' << Phi[i][j] << ' ' << W[i][j] << ' ' << B[i][j] << ' ' << A[i][j] << endl;
//             }
//             outfile.close();
//             outfile2.close();
//         }
//         }
//
