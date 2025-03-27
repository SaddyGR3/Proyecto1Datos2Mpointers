#include <iostream>
#include "MPointers.h"


int main() {
    try {
        // 1. Inicialización
        MPointer<int>::Init("localhost:50051");

        // 2. Crear un MPointer
        MPointer<int> ptr = MPointer<int>::New();
        std::cout << "MPointer creado exitosamente\n";

        // 3. Asignar valor - FORMA CORRECTA
        ptr = 42;  // Usando el operador= sobrecargado
        std::cout << "Valor 42 asignado correctamente\n";

        // 4. Leer valor - FORMA CORRECTA
        int valor = *ptr;  // Usando el operador* sobrecargado
        std::cout << "Valor recuperado: " << valor << "\n";


        MPointer<int> ptr2 = MPointer<int>::New();
        std::cout << "MPointer creado exitosamente\n";

        // 3. Asignar valor - FORMA CORRECTA
        ptr2 = 55 ;  // Usando el operador= sobrecargado
        std::cout << "Valor 55 asignado correctamente\n";

        // 4. Leer valor - FORMA CORRECTA
        int valor2 = *ptr2;  // Usando el operador* sobrecargado
        std::cout << "Valor recuperado: " << valor2 << "\n";

        // 2. Crear un MPointer
        MPointer<float> ptr3 = MPointer<float>::New();
        std::cout << "MPointer creado exitosamente\n";

        // 3. Asignar valor - FORMA CORRECTA
        ptr2 = 55.44234 ;  // Usando el operador= sobrecargado
        std::cout << "Valor 55 asignado correctamente\n";

        // 4. Leer valor - FORMA CORRECTA
        int valor3 = *ptr3;  // Usando el operador* sobrecargado
        std::cout << "Valor recuperado: " << valor3 << "\n";

        std::cout << "\n[DEBUG] Todas las pruebas completadas\n";
        return 0;



    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}