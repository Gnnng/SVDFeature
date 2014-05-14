#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>

#define MAXU 100
#define MAXI 1000
#define MAXFACTOR 20

double vec_product(double *a, double* b, int len) {
	double product = 0;
	for (int i = 0; i < len; ++i)
	{
		product += a[i] * b[i];
	}
	return product;
}

int main() {
	using namespace std;
	double avg = 43.43478260869565;
	double lambda = 0.004;
	int len;
	double 
		bu[MAXU],
		bi[MAXI],
		umat[MAXU][MAXFACTOR],
		imat[MAXI][MAXFACTOR];
	int 
		u_len, i_len, factor_len;

	ifstream 
		f_u_bias("u_bias.txt"), 
		f_i_bias("i_bias.txt"), 
		f_w_user("w_user.txt"), 
		f_w_item("w_item.txt");
	
	f_u_bias >> u_len;
	for(int i = 0 ; i < u_len; ++i) {
		f_u_bias >> bu[i];
	}
	f_u_bias.close();

	f_i_bias >> i_len;
	for(int i = 0; i < i_len; ++i) {
		f_i_bias >> bi[i];
	}
	f_i_bias.close();

	f_w_user >> u_len >> factor_len;
	for (int i = 0; i < u_len; ++i)
	{
		for (int j = 0; j < factor_len; ++j)
		{
			f_w_user >> umat[i][j];
		}
	}
	f_w_user.close();

	f_w_item >> u_len >> factor_len;
	for (int i = 0; i < u_len; ++i)
	{
		for (int j = 0; j < factor_len; ++j)
		{
			f_w_item >> imat[i][j];
		}
	}
	f_w_item.close();

	double sum = 0;
	FILE *f_base;
	f_base = fopen("art.base", "r");
	for(int i = 0; i < i_len * 3; ++i) {
		double score;
		int user, item;
		int empty;
		string s;
		char str[100];
		fscanf(f_base, "%lf 0 1 1 %d:1 %d:1", &score, &user, &item);
		double term1 = score - avg - bu[user] - bi[item] - vec_product(umat[user], imat[item], factor_len);
		sum += term1 * term1;
		sum += lambda * ( 
			vec_product(umat[user], umat[user], factor_len) + 
			vec_product(imat[item], imat[item], factor_len) + 
			bu[user] * bu[user] +
			bi[item] * bi[item]);
	}
	fclose(f_base);
	cout << sum;
}