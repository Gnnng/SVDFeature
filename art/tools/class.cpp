#include <iostream>  
#include <sstream>  
#include <fstream>  
#include <vector>  
#include <math.h>  
#include <stdlib.h>  
using namespace std;  
//存放元组的属性信息  
typedef vector<double> Tuple;//存储每条数据记录  
  
int dataNum;//数据集中数据记录数目  
int dimNum;//每条记录的维数  
int k;
ofstream tu("tu.txt");  
//计算两个元组间的欧几里距离  
double getDistXY(const Tuple& t1, const Tuple& t2)   
{  
    double sum = 0;  
    double t1l=0,t2l=0;
    for(int i=1; i<=dimNum; ++i)  
    {  
        sum += (t1[i]*t2[i]);
        t1l+=(t1[i]*t1[i]);
        t2l+=(t2[i]*t2[i]);  
    }  
    //cout<<sum/(sqrt(t1l)*sqrt(t2l))<<endl;
    return sum/(sqrt(t1l)*sqrt(t2l));  
}  
  
//根据质心，决定当前元组属于哪个簇  
int clusterOfTuple(Tuple means[],const Tuple& tuple){  
    double dist=getDistXY(means[0],tuple);  
    double tmp;  
    int label=0;//标示属于哪一个簇  
    for(int i=1;i<k;i++){  
        tmp=getDistXY(means[i],tuple);  
        if(tmp>dist) {dist=tmp;label=i;}  
    }  
    return label;     
}  
//获得给定簇集的平方误差  
double getVar(vector<Tuple> clusters[],Tuple means[]){  
    double var = 0;  
    for (int i = 0; i < k; i++)  
    {  
        vector<Tuple> t = clusters[i];  
        for (int j = 0; j< t.size(); j++)  
        {  
            var += getDistXY(t[j],means[i]);  
        }  
    }  
    //cout<<"sum:"<<sum<<endl;  
    return var;  
  
}  
//获得当前簇的均值（质心）  
Tuple getMeans(const vector<Tuple>& cluster){  
      
    int num = cluster.size();  
    Tuple t(dimNum+1, 0);  
    for (int i = 0; i < num; i++)  
    {  
        for(int j=1; j<=dimNum; ++j)  
        {  
            t[j] += cluster[i][j];  
        }  
    }  
    for(int j=1; j<=dimNum; ++j)  
        t[j] /= num;  
    return t;  
    //cout<<"sum:"<<sum<<endl;  
}  
  
void print(const vector<Tuple> clusters[])  
{  
    ofstream res("result.txt");
    for(int lable=0; lable<k; lable++)  
    {  
        res<<"第"<<lable+1<<"个簇："<<endl;  
        vector<Tuple> t = clusters[lable];  
        for(int i=0; i<t.size(); i++)  
        {  
            res<<i+1<<".(";  
            for(int j=0; j<=dimNum; ++j)  
            {  
                res<<t[i][j]<<", ";  
            }  
            res<<")\n";  
        }  
    }  
}  
  
void KMeans(vector<Tuple>& tuples){  
    vector<Tuple> clusters[k];//k个簇  
    Tuple means[k];//k个中心点  
    int i=0;  
    //一开始随机选取k条记录的值作为k个簇的质心（均值）  
    srand((unsigned int)time(NULL));  
    for(i=0;i<k;){  
        int iToSelect = rand()%tuples.size();  
        if(means[iToSelect].size() == 0)  
        {  
            for(int j=0; j<=dimNum; ++j)  
            {  
                means[i].push_back(tuples[iToSelect][j]);  
            }  
            ++i;  
        }  
    }  
    int lable=0;  
    //根据默认的质心给簇赋值  
    for(i=0;i!=tuples.size();++i){  
        lable=clusterOfTuple(means,tuples[i]);  
        clusters[lable].push_back(tuples[i]);  
    }  
    double oldVar=-1;  
    double newVar=getVar(clusters,means);  
    cout<<"初始的的整体误差平方和为："<<newVar<<endl;   
    int t = 0;  
    while(abs(newVar - oldVar) >= 1) //当新旧函数值相差不到1即准则函数值不发生明显变化时，算法终止  
    {  
        cout<<"第 "<<++t<<" 次迭代开始："<<endl;  
        for (i = 0; i < k; i++) //更新每个簇的中心点  
        {  
            means[i] = getMeans(clusters[i]);  
        }  
        oldVar = newVar;  
        newVar = getVar(clusters,means); //计算新的准则函数值  
        for (i = 0; i < k; i++) //清空每个簇  
        {  
            clusters[i].clear();  
        }  
        //根据新的质心获得新的簇  
        for(i=0; i!=tuples.size(); ++i){  
            lable=clusterOfTuple(means,tuples[i]);  
            clusters[lable].push_back(tuples[i]);  
        }  
        cout<<"此次迭代之后的整体误差平方和为："<<newVar<<endl;   
    }  
    
    tu<<newVar<<endl; 
    for (int i=0;i<k;i++)
    {
        for (int j=1;j<=8;j++)
        tu<<means[i][j]<<"\t";
        tu<<endl;
    }
    cout<<"The result is:\n";  
    print(clusters);  
}  
int main(int argc,char *argv[]){  
  
    char fname[256];  
    
    cout<<"请输入存放数据的文件名： ";  
    cin>>fname;  
    cout<<endl<<" 请依次输入: 维数 样本数目"<<endl;  
    cout<<endl<<" 维数dimNum: ";  
    cin>>dimNum;  
    cout<<endl<<" 样本数目dataNum: ";  
    cin>>dataNum;  
    for (k=1;k<=50;k++)
    {
    ifstream infile(fname);  
    if(!infile){  
        cout<<"不能打开输入的文件"<<fname<<endl;  
        return 0;  
    }  
    vector<Tuple> tuples;  
    //从文件流中读入数据  
    for(int i=0; i<dataNum && !infile.eof(); ++i)  
    {  
        string str;  
        getline(infile, str);  
        istringstream istr(str);  
        Tuple tuple(dimNum+1, 0);//第一个位置存放记录编号，第2到dimNum+1个位置存放实际元素  
        tuple[0] = i+1;  
        for(int j=1; j<=dimNum; ++j)  
        {  
            istr>>tuple[j];  
        }  
        tuples.push_back(tuple);  
    }  
  
    cout<<endl<<"开始聚类"<<endl;  
    KMeans(tuples);  
    }
    system("pause");
    return 0;  
}  
