#include <gtest/gtest.h>
#include "MemoryManagerProgram.cpp"
#include <string>
#include <limits>

class MemoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = new MemoryManagerProgram(1); // 1 MB de memoria
        std::cout << "\n========================================\n";
        std::cout << "Configurando pruebas para MemoryManager\n";
        std::cout << "========================================\n";
    }

    void TearDown() override {
        std::cout << "\n========================================\n";
        std::cout << "Limpiando después de las pruebas\n";
        std::cout << "========================================\n";
        delete manager;
    }

    MemoryManagerProgram* manager;
};

// Prueba específica para tipo int
TEST_F(MemoryManagerTest, IntTypeTest) {
    std::cout << "\n[TEST] Probando tipo int\n";

    int testValue = 42;
    std::cout << "Asignando bloque para int (" << sizeof(int) << " bytes)...\n";
    int id = manager->allocate(sizeof(int), "int");
    ASSERT_NE(id, -1) << "Fallo al asignar memoria para int";

    std::cout << "Escribiendo valor: " << testValue << "\n";
    manager->setValue<int>(id, testValue);

    std::cout << "Leyendo valor...\n";
    int retrievedValue = manager->getValue<int>(id);

    std::cout << "Valor recuperado: " << retrievedValue << "\n";
    ASSERT_EQ(testValue, retrievedValue) << "El valor recuperado no coincide";

    std::cout << "[PASS] Prueba int completada con éxito\n";
}

// Prueba específica para tipo float
TEST_F(MemoryManagerTest, FloatTypeTest) {
    std::cout << "\n[TEST] Probando tipo float\n";

    float testValue = 3.14159f;
    std::cout << "Asignando bloque para float (" << sizeof(float) << " bytes)...\n";
    int id = manager->allocate(sizeof(float), "float");
    ASSERT_NE(id, -1) << "Fallo al asignar memoria para float";

    std::cout << "Escribiendo valor: " << testValue << "\n";
    manager->setValue<float>(id, testValue);

    std::cout << "Leyendo valor...\n";
    float retrievedValue = manager->getValue<float>(id);

    std::cout << "Valor recuperado: " << retrievedValue << "\n";
    ASSERT_FLOAT_EQ(testValue, retrievedValue) << "El valor recuperado no coincide";

    std::cout << "[PASS] Prueba float completada con éxito\n";
}

// Prueba específica para tipo double
TEST_F(MemoryManagerTest, DoubleTypeTest) {
    std::cout << "\n[TEST] Probando tipo double\n";

    double testValue = 2.718281828459045;
    std::cout << "Asignando bloque para double (" << sizeof(double) << " bytes)...\n";
    int id = manager->allocate(sizeof(double), "double");
    ASSERT_NE(id, -1) << "Fallo al asignar memoria para double";

    std::cout << "Escribiendo valor: " << testValue << "\n";
    manager->setValue<double>(id, testValue);

    std::cout << "Leyendo valor...\n";
    double retrievedValue = manager->getValue<double>(id);

    std::cout << "Valor recuperado: " << retrievedValue << "\n";
    ASSERT_DOUBLE_EQ(testValue, retrievedValue) << "El valor recuperado no coincide";

    std::cout << "[PASS] Prueba double completada con éxito\n";
}

// Prueba específica para tipo char
TEST_F(MemoryManagerTest, CharTypeTest) {
    std::cout << "\n[TEST] Probando tipo char\n";

    char testValue = 'Z';
    std::cout << "Asignando bloque para char (" << sizeof(char) << " byte)...\n";
    int id = manager->allocate(sizeof(char), "char");
    ASSERT_NE(id, -1) << "Fallo al asignar memoria para char";

    std::cout << "Escribiendo valor: '" << testValue << "'\n";
    manager->setValue<char>(id, testValue);

    std::cout << "Leyendo valor...\n";
    char retrievedValue = manager->getValue<char>(id);

    std::cout << "Valor recuperado: '" << retrievedValue << "'\n";
    ASSERT_EQ(testValue, retrievedValue) << "El valor recuperado no coincide";

    std::cout << "[PASS] Prueba char completada con éxito\n";
}

// Prueba específica para tipo string
TEST_F(MemoryManagerTest, StringTypeTest) {
    std::cout << "\n[TEST] Probando tipo string\n";

    std::string testValue = "Hola mundo desde pruebas unitarias";
    std::cout << "Asignando bloque para string (" << testValue.size() << " bytes)...\n";
    int id = manager->allocate(testValue.size(), "string");
    ASSERT_NE(id, -1) << "Fallo al asignar memoria para string";

    std::cout << "Escribiendo valor: \"" << testValue << "\"\n";
    manager->setValue<std::string>(id, testValue);

    std::cout << "Leyendo valor...\n";
    std::string retrievedValue = manager->getValue<std::string>(id);

    std::cout << "Valor recuperado: \"" << retrievedValue << "\"\n";
    ASSERT_EQ(testValue, retrievedValue) << "El string recuperado no coincide";

    std::cout << "[PASS] Prueba string completada con éxito\n";
}

// Prueba de valores límite
TEST_F(MemoryManagerTest, BoundaryValuesTest) {
    std::cout << "\n[TEST] Probando valores límite\n";

    // Prueba con int
    std::cout << "Probando INT_MAX...\n";
    int maxInt = std::numeric_limits<int>::max();
    int intId = manager->allocate(sizeof(int), "int");
    manager->setValue<int>(intId, maxInt);
    ASSERT_EQ(maxInt, manager->getValue<int>(intId));

    // Prueba con float
    std::cout << "Probando FLT_MIN...\n";
    float minFloat = std::numeric_limits<float>::min();
    int floatId = manager->allocate(sizeof(float), "float");
    manager->setValue<float>(floatId, minFloat);
    ASSERT_FLOAT_EQ(minFloat, manager->getValue<float>(floatId));

    std::cout << "[PASS] Prueba de valores límite completada con éxito\n";
}

// Prueba de manejo de errores
TEST_F(MemoryManagerTest, ErrorHandlingTest) {
    std::cout << "\n[TEST] Probando manejo de errores\n";

    std::cout << "Intentando obtener valor de ID no existente...\n";
    EXPECT_THROW({
        try {
            manager->getValue<int>(999);
        } catch(const std::runtime_error& e) {
            std::cout << "Error esperado capturado: " << e.what() << "\n";
            throw;
        }
    }, std::runtime_error);

    std::cout << "Intentando escribir en ID no existente...\n";
    EXPECT_THROW({
        try {
            manager->setValue<int>(999, 42);
        } catch(const std::runtime_error& e) {
            std::cout << "Error esperado capturado: " << e.what() << "\n";
            throw;
        }
    }, std::runtime_error);

    std::cout << "Intentando asignar bloque demasiado grande...\n";
    int id = manager->allocate(2 * 1024 * 1024, "int"); // 2MB cuando solo hay 1MB
    ASSERT_EQ(id, -1) << "Se esperaba que fallara la asignación de bloque grande";

    std::cout << "[PASS] Prueba de manejo de errores completada con éxito\n";
}

int main(int argc, char** argv) {
    std::cout << "========================================\n";
    std::cout << "INICIANDO PRUEBAS UNITARIAS COMPLETAS\n";
    std::cout << "========================================\n";

    ::testing::InitGoogleTest(&argc, argv);
    auto result = RUN_ALL_TESTS();

    std::cout << "\n========================================\n";
    std::cout << "PRUEBAS COMPLETADAS - RESUMEN\n";
    std::cout << "========================================\n";

    return result;
}