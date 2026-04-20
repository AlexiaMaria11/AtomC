struct Pt {
    int x;
    int y;
}; // Eroare 1: Lipsește ';' după definiția structurii

struct Pt points[10]; // Eroare 2: Lipsește ';' după declararea variabilei globale

double max(double a, double b){ // Eroare 3: Lipsește virgula între parametri
    if (a > b) return a; // Eroare 4: Lipsesc parantezele obligatorii la 'if'
        else return b;
    }

int len(char s[ ]){
    int i;
    i = 0;
    while(s[i]) { // Eroare 5: Lipsește închiderea parantezei de la 'while'
        i = i + 1;
    }
    return i;
    }

void main(){
    int i;
    i = 10;
    while(i != 0){
        puti(i); // Eroare 7: Lipsește ';' după apelul funcției
        i = i / 2; // Eroare 8: Expresie incompletă după operatorul de împărțire
        }
}
    // Eroare 9: Lipsește acolada de închidere a funcției main