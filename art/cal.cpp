#include <iostream>
#include <cstdio>
#include <fstream>
using namespace std;
int x;
double cj(double *a,double *b)
{
    double ret=0;
    for (int i=0;i<x;i++)
    {
        ret+=a[i]*b[i];
    }
    return ret;
}
double mo(double *a)
{
    double ret=0;
    for (int i=0;i<x;i++)
    {
        ret+=a[i]*a[i];
    }
    return ret;
}
int main()
{
	ifstream fpu("w_user.txt"),fpi("w_item.txt"),fbu("u_bias.txt"),fbi("i_bias.txt"),ori("art1.base");
	int n;
	double lem=0.004;
	fpu>>n;
	fpu>>x;
	int mu=n;
	int i,j;
	double pu[1000][10],pi[1000][10],bu[1000],bi[1000];
	for (i=0;i<n;i++)
	{
		for (j=0;j<x;j++)
		{
			fpu>>pu[i][j];
		}
	}
	fpi>>n;
	fpi>>x;
	int mi=n;
	for (i=0;i<n;i++)
	{
		for (j=0;j<x;j++)
		{
			fpi>>pi[i][j];
		}
	}
	fbu>>n;
	for (i=0;i<n;i++)
	{
		fbu>>bu[i];
		
	}
	fbi>>n;
	for (i=0;i<n;i++)
	{
		fbi>>bi[i];
		
	}
	double avg=0;
	double sum=0;
	int len;
	double origin[10000];
	int ux[10000],ix[10000];
	ori>>len;
	for (i=0;i<len;i++)
	{
		ori>>origin[i];
		int j;
		ori>>j>>j>>j;
		ori>>ux[i];
		ori>>j;
		ori>>ix[i];
		ori>>j;
		avg+=origin[i]/(double)len;
		//cout<<origin[i]<<","<<ux[i]<<","<<ix[i]<<endl;
	}
	cout<<avg<<endl;
	for (i=0;i<len;i++)
	{
		double xbu=bu[ux[i]],xbi=bi[ix[i]];
		double *xpi=pi[ix[i]],*xpu=pu[ux[i]];
		if (i==0)
		cout<<xpu[0]<<":"<<xpi[0]<<endl;
		sum=sum+(origin[i]-avg-xbu-xbi-cj(xpu,xpi))*(origin[i]-avg-xbu-xbi-cj(xpu,xpi))+lem*(cj(xpu,xpu)+cj(xpi,xpi)+xbu*xbu+xbi*xbi);
	}
	cout<<sum;
	cin>>x;
}
 
