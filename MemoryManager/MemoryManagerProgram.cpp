#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <memory>

class MemoryBlock {
public:
    void* address;
    size_t size;
    std::string type;

    MemoryBlock(void* address, size_t size, std::string type = "int")
        : address(address), size(size), type(type) {}

    // Método genérico para tipos POD (versión const)
    template <typename T>
    typename std::enable_if<std::is_pod<T>::value, T>::type
    getValue() const {
        if (!address) throw std::runtime_error("Intento de leer memoria NULL");
        if (!isTypeValid<T>()) {
            throw std::runtime_error("Tipo de dato incorrecto para este bloque");
        }
        if (sizeof(T) > size) throw std::runtime_error("Tamaño del dato excede el tamaño del bloque");

        T value;
        std::memcpy(&value, address, sizeof(T));
        return value;
    }

    // Método para strings (versión const)
    std::string getStringValue() const {
        if (!address) throw std::runtime_error("Intento de leer memoria NULL");
        if (type != "string") throw std::runtime_error("No es un bloque de string");

        // Cambio clave: usar strnlen para encontrar el null terminator
        const char* str = static_cast<const char*>(address);
        size_t actual_length = strnlen(str, size); // No leerá más allá del null terminator

        return std::string(str, actual_length);
    }

    // Método genérico para asignar valores POD
    template <typename T>
    typename std::enable_if<std::is_pod<T>::value>::type
    setValue(const T& value) {
        if (!address) throw std::runtime_error("Intento de escribir en memoria NULL");
        if (!isTypeValid<T>()) {
            throw std::runtime_error("Tipo de dato incorrecto para este bloque");
        }
        if (sizeof(T) > size) throw std::runtime_error("Tamaño del dato excede el tamaño del bloque");
        std::memcpy(address, &value, sizeof(T));
    }

    // Método para asignar strings
    void setStringValue(const std::string& value) {
        if (!address) throw std::runtime_error("Intento de escribir en memoria NULL");
        if (type != "string") throw std::runtime_error("No es un bloque de string");
        if (value.size() >= size) throw std::runtime_error("Tamaño del string excede el tamaño del bloque");

        // Cambio clave: copiar el string y agregar null terminator explícito
        std::memcpy(address, value.data(), value.size());
        static_cast<char*>(address)[value.size()] = '\0'; // Asegurar null terminator
    }

private:
    // Verifica la compatibilidad de tipos (versión const)
    template <typename T>
    bool isTypeValid() const {
        if (type == "int" && std::is_same<T, int>::value) return true;
        if (type == "float" && std::is_same<T, float>::value) return true;
        if (type == "double" && std::is_same<T, double>::value) return true;
        if (type == "char" && std::is_same<T, char>::value) return true;
        if (type == "bool" && std::is_same<T, bool>::value) return true;
        if (type == "short" && std::is_same<T, short>::value) return true;
        if (type == "long" && std::is_same<T, long>::value) return true;
        if (type == "long long" && std::is_same<T, long long>::value) return true;
        if (type == "unsigned" && std::is_same<T, unsigned>::value) return true;
        return false;
    }
};

class MemoryMap {
public:
    int id;
    size_t size;
    std::string type;
    int refcount;
    MemoryBlock block;
    bool initialized;

    MemoryMap(int id, size_t size, void* address, std::string type = "int")
        : block(address, size, type), id(id), size(size), type(type), refcount(1), initialized(false) {}
};

class MemoryManagerProgram {
    size_t totalMemory;
    std::vector<MemoryMap> memoryTable;
    char* memory;

public:
    MemoryManagerProgram(size_t sizeMB) {
        totalMemory = sizeMB * 1'000'000; // Convert MB to Bytes
        memory = (char*)std::malloc(totalMemory);
        if (!memory) {
            throw std::bad_alloc();
        }
        freeList.push_back({memory, totalMemory});
    }

    ~MemoryManagerProgram() {
        std::free(memory);
    }

    // Asigna memoria para un tipo específico
    int allocate(size_t size, std::string type = "int") {
        static int nextId = 1;

        // Verificar tamaño mínimo según el tipo
        size_t minSize = getMinSizeForType(type);
        if (size < minSize) {
            throw std::runtime_error("Tamaño muy pequeño para el tipo " + type);
        }

        void* addr = findFreeSpace(size);
        if (!addr) {
            compactMemory();
            addr = findFreeSpace(size);
            if (!addr) return -1;
        }

        memoryTable.push_back(MemoryMap(nextId, size, addr, type));
        return nextId++;
    }

    // Obtiene el tipo de un bloque (versión const)
    std::string getBlockType(int id) const {
        for (const auto& block : memoryTable) {
            if (block.id == id) {
                return block.type;
            }
        }
        throw std::runtime_error("ID no encontrado");
    }

    // Obtiene la dirección de un bloque (versión const)
    void* getBlockAddress(int id) const {
        for (const auto& block : memoryTable) {
            if (block.id == id) {
                return block.block.address;
            }
        }
        throw std::runtime_error("ID no encontrado");
    }

    // Obtiene el tamaño de un bloque (versión const)
    size_t getBlockSize(int id) const {
        for (const auto& block : memoryTable) {
            if (block.id == id) {
                return block.size;
            }
        }
        throw std::runtime_error("ID no encontrado");
    }

    // Asigna un valor a un bloque
    template <typename T>
    void setValue(int id, const T& value) {
        for (auto& block : memoryTable) {
            if (block.id == id) {
                if constexpr (std::is_same_v<T, std::string>) {
                    if (block.type != "string") {
                        throw std::runtime_error("El bloque no es de tipo string");
                    }
                    block.block.setStringValue(value);
                } else {
                    if (!std::is_pod_v<T>) {
                        throw std::runtime_error("Solo soporta tipos string y pod");
                    }
                    block.block.setValue(value);
                }
                block.initialized = true;
                return;
            }
        }
        throw std::runtime_error("ID no encontrado");
    }

    // Obtiene un valor de un bloque (versión const)
    template <typename T>
    T getValue(int id) const {
        for (const auto& block : memoryTable) {
            if (block.id == id) {
                if (!block.initialized) {
                    throw std::runtime_error("Intento de leer bloque in inicializar");
                }
                if constexpr (std::is_same_v<T, std::string>) {
                    if (block.type != "string") {
                        throw std::runtime_error("el bloque no es de tipo string");
                    }
                    return block.block.getStringValue();
                } else {
                    if (!std::is_pod_v<T>) {
                        throw std::runtime_error("Solo soporta tipos string y pod");
                    }
                    return block.block.getValue<T>();
                }
            }
        }
        throw std::runtime_error("ID no encontrado");
    }

    // Incrementa el contador de referencias
    int increaseRefCount(int id) {
        for (auto& block : memoryTable) {
            if (block.id == id) {
                return ++block.refcount;
            }
        }
        throw std::runtime_error("ID no encontrado");
    }

    // Decrementa el contador de referencias
    int decreaseRefCount(int id) {
        for (auto& block : memoryTable) {
            if (block.id == id) {
                if (--block.refcount == 0) {
                    freeMemory(id);
                    return 0;
                }
                return block.refcount;
            }
        }
        throw std::runtime_error("ID no encontrado");
    }

    // Compacta la memoria
    void compactMemory() {
        std::sort(memoryTable.begin(), memoryTable.end(), [](const MemoryMap& a, const MemoryMap& b) {
            return a.block.address < b.block.address;
        });

        char* current = memory;
        for (auto& block : memoryTable) {
            if (block.block.address != current) {
                std::memmove(current, block.block.address, block.size);
                block.block.address = current;
            }
            current += block.size;
        }

        freeList.clear();
        if (current < memory + totalMemory) {
            freeList.push_back({current, static_cast<size_t>(memory + totalMemory - current)});
        }
    }

private:
    struct FreeBlock {
        void* address;
        size_t size;
    };

    std::vector<FreeBlock> freeList;

    // Encuentra espacio libre
    void* findFreeSpace(size_t size) {
        for (auto it = freeList.begin(); it != freeList.end(); ++it) {
            if (it->size >= size) {
                void* addr = it->address;
                if (it->size > size) {
                    it->address = static_cast<char*>(it->address) + size;
                    it->size -= size;
                } else {
                    freeList.erase(it);
                }
                return addr;
            }
        }
        return nullptr;
    }

    // Libera memoria
    void freeMemory(int id) {
        auto it = std::remove_if(memoryTable.begin(), memoryTable.end(),
            [id](const MemoryMap& block) { return block.id == id; });

        if (it != memoryTable.end()) {
            FreeBlock freedBlock = {it->block.address, it->size};
            freeList.push_back(freedBlock);
            memoryTable.erase(it, memoryTable.end());
            mergeFreeBlocks();
        }
    }

    // Fusiona bloques libres adyacentes
    void mergeFreeBlocks() {
        if (freeList.empty()) return;

        std::sort(freeList.begin(), freeList.end(), [](const FreeBlock& a, const FreeBlock& b) {
            return a.address < b.address;
        });

        for (size_t i = 0; i < freeList.size() - 1; ) {
            FreeBlock& current = freeList[i];
            FreeBlock& next = freeList[i + 1];

            if (static_cast<char*>(current.address) + current.size == next.address) {
                current.size += next.size;
                freeList.erase(freeList.begin() + i + 1);
            } else {
                i++;
            }
        }
    }

    // Obtiene el tamaño mínimo para un tipo
    size_t getMinSizeForType(const std::string& type) const {
        if (type == "int") return sizeof(int);
        if (type == "float") return sizeof(float);
        if (type == "double") return sizeof(double);
        if (type == "char") return sizeof(char);
        if (type == "bool") return sizeof(bool);
        if (type == "short") return sizeof(short);
        if (type == "long") return sizeof(long);
        if (type == "long long") return sizeof(long long);
        if (type == "unsigned") return sizeof(unsigned);
        if (type == "string") return 1; // Minimum string size is 1 byte
        return 1; // Default minimum size
    }
};