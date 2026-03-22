struct Point{
    int x;
    int y;
};

int sum(int a, int b){
    return a + b;
}

void demo(){
    int x, y;
    double d1, d2, d3;
    char c1, c2, c3;
    char *s1, *s2;
    struct Point p;

    x = 10;
    y = 12;

    d1 = 3.14;
    d2 = 49e-1;
    d3 = 0.25E+2;

    c1 = 'a';
    c2 = '\n';
    c3 = '\'';

    s1 = "text simplu";
    s2 = "linie\n tab\t ghilimele \" si slash \\";

    p.x = x;
    p.y = y;

    if(x < y && d1 >= 2.0){
        x = x + 1;
    }else{
        x = x - 1;
    }

    if(x == y || x != 0){
        x = sum(x, y);
    }

    while(!(x <= 0)){
        x = x - 1;
    }

    // comentariu pe o singura linie
    // 12345 [] {} () , ; . + - * / == != <= >= && || !

    return;
}

int main(){
    demo();
    return 0;
}
