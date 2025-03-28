#ifndef MPOINTERS_H
#define MPOINTERS_H

#include <grpcpp/grpcpp.h>
#include "memory_manager.grpc.pb.h"
#include <memory>
#include <string>
#include <typeinfo>
#include <cstring>
#include <type_traits>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using memorymanager::MemoryManager;
using memorymanager::CreateRequest;
using memorymanager::CreateResponse;
using memorymanager::SetRequest;
using memorymanager::SetResponse;
using memorymanager::GetRequest;
using memorymanager::GetResponse;
using memorymanager::RefCountRequest;
using memorymanager::RefCountResponse;
using memorymanager::DataType;

class MPointerBase {
protected:
    static std::shared_ptr<MemoryManager::Stub> stub_;  // Mover el stub a la clase base
public:
    static void Init(const std::string& server_address);  // Mover Init a la clase base
    virtual ~MPointerBase() = default;
    virtual int getId() const = 0;
};

template <typename T>
class MPointer : public MPointerBase {
private:
    int id_;
    // Eliminar el stub_ de aquí, ahora está en la clase base
    mutable T cached_value_;
    mutable bool dirty_;

    void fetchValue() const;
    void storeValue() const;
    void increaseRefCount();
    void decreaseRefCount();

    // Mapeo de tipos
    static DataType getProtoType() {
        if constexpr (std::is_same_v<T, int32_t>) return DataType::INT;
        if constexpr (std::is_same_v<T, float>) return DataType::FLOAT;
        if constexpr (std::is_same_v<T, char>) return DataType::CHAR;
        if constexpr (std::is_same_v<T, std::string>) return DataType::STRING;
        throw std::runtime_error("Unsupported type");
    }

    // Tamaño del tipo
    static size_t getTypeSize() {
        if constexpr (std::is_same_v<T, std::string>) return 64; // Tamaño por defecto para strings
        return sizeof(T);
    }

public:
    static void Init(const std::string& server_address);
    static MPointer<T> New(size_t size = 0);

    MPointer() : id_(-1), cached_value_(), dirty_(false) {}
    explicit MPointer(int id);
    MPointer(const MPointer& other);
    MPointer(MPointer&& other) noexcept;
    ~MPointer();

    MPointer<T>& operator=(const MPointer<T>& other);
    MPointer<T>& operator=(MPointer<T>&& other) noexcept;
    MPointer<T>& operator=(const T& value);

    T& operator*();
    const T& operator*() const;
    T* operator->();
    const T* operator->() const;

    bool operator==(std::nullptr_t) const { return id_ == -1; }
    bool operator!=(std::nullptr_t) const { return id_ != -1; }

    explicit operator bool() const { return id_ != -1; }
    int getId() const override { return id_; }
};

// Declaración de especializaciones
template <>
void MPointer<std::string>::fetchValue() const;
template <>
void MPointer<std::string>::storeValue() const;



#endif // MPOINTERS_H