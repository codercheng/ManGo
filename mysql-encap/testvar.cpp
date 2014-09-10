#include <iostream>

using namespace std;

class TestVar{
public:
	TestVar():b(2), e(a){
		a = 1;	
		cout<<a<<" "<<b<<" "<<c<<" "<<d<<" "<<e<<" "<<f<<" "<<h<<endl;
	}
private:
	//int a = 0;//error: ISO C++ forbids initialization of member ‘a’
	int a;//初始化可以在构造函数里面，也可以在初始化列表
	//const int b = 0;//error: ISO C++ forbids initialization of member ‘a’
	const int b;//常量的正确初始化应该使用[构造函数的初始化列表]
	int &e;//引用类型变量，和常量一样只能在[构造函数的初始化列表],参数是同类型变量
	
	//static int c = 0;//ISO C++ forbids in-class initialization of non-const static member ‘c’
	static int c;
	static const int d = 4;//注意只有整型数据才行[int.., char]
	static const char f = 'a';//可以，char也是整型数据
	static const double h = 9.9;//书上说不行，但是在g++下是可以的，根编译器有关吧
	
};

int TestVar::c = 3;//静态成员的正确初始化方法


int main(){
	TestVar t;
	return 0;
}
