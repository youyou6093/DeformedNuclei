//
//  Solution.h
//  deform_c++
//
//  Created by Junjie Yang on 6/18/17.
//  Copyright © 2017 Junjie Yang. All rights reserved.
//


#ifndef Solution_h
#define Solution_h
#include "symm.h"
#include "generate_state.h"
#include<vector>
#include "integrator.h"
#include "states.cpp"
extern int max_L;
extern int N;
extern int nodes;
extern unordered_map<string, vector<vector<double>> > States_map;
using std::vector;
using namespace std;

struct eig2{
    eig solution;
    int m;
};

/*compare function for eig2 struct*/
bool compare_eig2(eig2 a,eig2 b){
    return (a.solution.eigen_values<b.solution.eigen_values);
}

/* Combine the coefficient of the eigenvector and
   the state together so I could form the wave function
   of the eigenvalues */
struct coef_pair{
    double coefs;
    State state;
};


/* A struct that stores wave function for one kappa
   channel*/
struct solution_wave_function{
    int kappa;
    vector<double>upper;
    vector<double>lower;
};

class Solution{
public:
    vector<coef_pair> my_pair;
    int m;     //the quantum number of this solution
    double energy;
    vector<int> kappas;
    State primary_state;
    vector<solution_wave_function> wavefunctions;
    
    Solution(){
        m=0;
        energy=0.0;
    }
    
    /*An eigenvalue from diagnolizing matrix*/
    //this constructor will give me the pair of eigenvectors and states
    //also it will provide all the kappas
    Solution(eig2 result){
        this->m = result.m;                                //m is the quantum numbrer
        energy=result.solution.eigen_values;               //Energy for the solution
        coef_pair temp;                                    //Just a holder for the pair
        vector<State> states=generate_statesm(m,max_L,nodes);    //Generate the basis for this m
        for(int i=0;i<states.size();i++){
            /* Avoiding small coefficients due to some numerical errors*/
            // if (abs(result.solution.eigen_vectors[i])>1e-5){
            	temp.coefs=result.solution.eigen_vectors[i];
            	temp.state=states[i];
            	my_pair.push_back(temp);
            	/*Every state has it's own kappa so I will add this state's
             	kappa into the solution*/
            	add_kappa(states[i].k);
            // }
            
        }
    }
    
    /*Decide whether add new kappa into collection*/
    bool add_kappa(int k){
        for(int i = 0;i<kappas.size();i++){
            if (k == kappas[i])
                return true;
        }
        kappas.push_back(k);
        return false;
    }
    

    /*this is more for debug purpose
      print all */
    void print_eigenvectors(){
    	get_all_wavefunction();
    	for(int i = 0; i < kappas.size(); i++){
    		double coef_for_kappa = 0;
    		for(int j = 0; j < my_pair.size(); j++){
    			if (my_pair[j].state.k == kappas[i])
    				coef_for_kappa += pow(my_pair[j].coefs, 2);
    		}
    		cout << "coef for " << kappas[i] << " is " << coef_for_kappa << endl;
    	}


    	/* the integration of this part*/
    	double norm = 0.0;
    	for( int i = 0 ; i < wavefunctions.size(); i++){
    		vector<double> inte;
    		inte.clear();
    		for(int j = 0; j < N; j++){
    			// cout << "something" <<endl;
    			inte.push_back(pow(wavefunctions[i].upper[j], 2) + pow(wavefunctions[i].lower[j], 2));
    			// inte.push_back(pow(wavefunctions[i].upper[j], 2));
    		}
    		norm += my_spline(inte, fx, my_tolerance).integral();
    	}
    	// cout << "normalization: " << norm <<endl; 

    }


    /*Find the primary state, make sure the primary states always have + sign, increase numerical stability*/
    void get_primary_state(){
        vector<State> states = generate_statesm(m, max_L, nodes);
        int max = 0;
        int sign = 1;
        for(int i=0;i<my_pair.size();i++){
            if (abs(my_pair[i].coefs)>abs(my_pair[max].coefs)){
                max=i;
            }
        }
        
        /* new modification, make sure the primary state is positive */
        if ((abs(my_pair[max].coefs) * my_pair[max].coefs) >= 0)
            sign = 1;
        else
            sign = -1;
        
        for(int i = 0; i < my_pair.size(); i++)
            my_pair[i].coefs *= sign;
            
        primary_state=my_pair[max].state;
        
        
    }
    
    
    /*get the wave function for one kappa*/
    solution_wave_function get_wave_function(int kappa){
        vector<double> g,f;
        vector<double> upper(N, 0.0);
        vector<double> lower(N, 0.0);
        unordered_map<string, vector<vector<double>>>::iterator States_ptr;
        coef_pair temp;

        //go through every wavefunction in this solution
        for(int i=0;i<my_pair.size();i++){
            temp=my_pair[i];               //get one element from the pair
            if(temp.state.k==kappa){       //if the kappa of that state is what I want
                States_ptr=States_map.find(temp.state.key);
                g=States_ptr->second[0];   //get the wavefunctions
                f=States_ptr->second[1];
                for(int j=0;j<N;j++){
                    upper[j]+=g[j]*temp.coefs;   //add it to our wavefunction
                    lower[j]+=f[j]*temp.coefs;
                }
            }
        }
        solution_wave_function wavefunction1;
        wavefunction1.kappa=kappa;
        wavefunction1.upper=upper;
        wavefunction1.lower=lower;
        return wavefunction1;
    }
    
    
    /*get the complete wavefunction, it will be stored in wavefunctions struct, has kappa, f and g*/
    void get_all_wavefunction(){               //get all wavefunctions that I need
        get_primary_state();                   //get primary state and correct the sign
        wavefunctions.clear();
        for(int i=0;i<kappas.size();i++){
            wavefunctions.push_back(get_wave_function(kappas[i]));
        }
    }
};


#endif /* Solution_h */
