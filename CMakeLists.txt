cmake_minimum_required(VERSION 3.14)
project(Datos2Project1Mpointers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar gRPC y Protobuf
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# Encontrar Google Test
find_package(GTest REQUIRED)

# Incluir directorios (agrega la carpeta LinkedList)
include_directories(
        ${Protobuf_INCLUDE_DIRS}
        ${gRPC_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/MemoryManager
        ${CMAKE_SOURCE_DIR}/generated
        ${CMAKE_SOURCE_DIR}/Mpointer
        ${CMAKE_SOURCE_DIR}/LinkedList  # Añade esta línea
        ${CMAKE_SOURCE_DIR}/tests
)

# Archivos generados por protoc
set(PROTO_FILES
        generated/memory_manager.pb.cc
        generated/memory_manager.grpc.pb.cc
)

# Compilar el servidor (MemoryManager)
add_executable(memory_manager_server
        memory_manager_server.cpp  # Main del servidor
        MemoryManager/MemoryManagerProgram.cpp  # Lógica del MemoryManager
        ${PROTO_FILES}  # Archivos generados por protoc
)
target_link_libraries(memory_manager_server PRIVATE gRPC::grpc++ protobuf::libprotobuf)

# linked_list executable (simplificado)
add_executable(linked_list
        LinkedList/LinkedList.cpp
        LinkedList/main.cpp
        LinkedList/Elemento.h
        Mpointer/MPointers.cpp
        Mpointer/MPointers.h
        generated/memory_manager.pb.cc
        generated/memory_manager.grpc.pb.cc
)
target_link_libraries(linked_list PRIVATE gRPC::grpc++ protobuf::libprotobuf)

# Compilar las pruebas unitarias
add_executable(memory_manager_tests
        tests/MemoryManagerTests.cpp  # Archivo de pruebas unitarias
        tests/ServerTest.cpp  # Archivo de pruebas unitarias
        MemoryManager/MemoryManagerProgram.cpp  # Lógica del MemoryManager
)
target_link_libraries(memory_manager_tests PRIVATE GTest::GTest GTest::Main)

# Habilitar pruebas en CTest (opcional)
enable_testing()
add_test(NAME memory_manager_tests COMMAND memory_manager_tests)
