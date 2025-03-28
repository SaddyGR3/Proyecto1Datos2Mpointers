#include <iostream>
#include <iomanip>
#include <string>
#include "MPointers.h"

int main() {
    try {
        // 1. Se inicializa una sola vez para todos los tipos
        std::cout << "Conectando con el servidor... ";
        MPointerBase::Init("localhost:50051");
        std::cout << "Conexion establecida!\n\n";

        // 2.Prueba con diferentes tipos de datos
        std::cout << "==== Prueba de MPointer ====\n";

        // Entero (int)
        {
            std::cout << "\n[Entero (int)]\n";
            MPointer<int> myint = MPointer<int>::New();
            myint = 25;
            std::cout << "Entero almacenado: " << *myint << "\n";
            std::cout << "ID del bloque: " << myint.getId() << "\n";
        }

        // Float
        {
            std::cout << "\n[Float]\n";
            MPointer<float> myfloat = MPointer<float>::New();
            myfloat = 23.7231f;
            std::cout << std::fixed << std::setprecision(4);
            std::cout << "Float almacenado: " << *myfloat << "\n";
            std::cout << "ID del bloque: " << myfloat.getId() << "\n";
        }

        // Char
        {
            std::cout << "\n[Char]\n";
            MPointer<char> mychar = MPointer<char>::New();
            mychar = 'G';
            std::cout << "Char Almacenado: " << *mychar << "\n";
            std::cout << "ID del bloque: " << mychar.getId() << "\n\n";
        }

        // String
        {
            std::cout << "[String]\n";
            MPointer<std::string> mensaje = MPointer<std::string>::New(50);
            mensaje = "Hola desde MPointer!";
            std::cout << "Mensaje: " << *mensaje << "\n";
            std::cout << "ID del bloque: " << mensaje.getId() << "\n\n";
        }



        // 3. Ejemplo de reutilización
        {
            std::cout << "==== Reasignacion ====\n";
            MPointer<int> contador = MPointer<int>::New();
            contador = 10;

            std::cout << "Contador inicial: " << *contador << "\n";

            *contador += 5;
            std::cout << "Contador modificado: " << *contador << "\n";

            MPointer<int> copia = contador;
            std::cout << "Copia del contador: " << *copia << "\n\n";
        }

        std::cout << "\n==== Prueba Completa ====\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << std::endl;
        return 1;
    }
}