#include "LinkedList.h"
#include <iostream>

int main() {
    try {
        std::cout << "Conectando con el servidor... ";
        MPointerBase::Init("localhost:50051");
        std::cout << "Conexion establecida!\n\n";

        // 2.Prueba con diferentes tipos de datos
        std::cout << "==== Prueba de MPointer ====\n";

        // Entero (int)
        std::cout << "\n[Entero (int)]\n";
        MPointer<int> myint = MPointer<int>::New();
        myint = 25;
        std::cout << "Entero almacenado: " << *myint << "\n";
        std::cout << "ID del bloque: " << myint.getId() << "\n";

        LinkedListInt lista;

        // 3. Operaciones básicas
        lista.push_back(10);
        lista.push_back(20);
        lista.push_front(5);
        lista.push_back(30);

        std::cout << "Lista completa: ";
        lista.print(); // [5, 10, 20, 30]+
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}