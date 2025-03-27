#include <iostream>
#include <iomanip>
#include "MPointers.h"

int main() {
    try {
        // Inicialización UNA sola vez para todos los tipos
        MPointerBase::Init("localhost:50051");

        // Ahora funciona con cualquier tipo sin necesidad de Init específico
        MPointer<float> fptr = MPointer<float>::New();
        fptr = 55.44234f;
        std::cout << "Float: " << std::setprecision(7) << *fptr << "\n";

        MPointer<int> iptr = MPointer<int>::New();
        iptr = 42;
        std::cout << "Int: " << *iptr << "\n";

        MPointer<char> cptr = MPointer<char>::New();
        cptr = 'A';
        std::cout << "Char: " << *cptr << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}