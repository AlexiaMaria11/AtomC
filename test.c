int f(int x,double y,char c){
	int r;
	double d;
	r=x||c;
	r=x&&c;
	r=x==c;
	r=x<y;
	d=x+y*2.0;
	return r;
	}

int main(){
	return f(1,2.0,'a');
	}
