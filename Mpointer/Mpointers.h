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

enum class MPointerType {
    INT,
    FLOAT,
    DOUBLE,
    CHAR,
    STRING,
    UNKNOWN
};

template <typename T>
MPointerType getMPointerType() {
    if (std::is_same<T, int>::value) return MPointerType::INT;
    if (std::is_same<T, float>::value) return MPointerType::FLOAT;
    if (std::is_same<T, double>::value) return MPointerType::DOUBLE;
    if (std::is_same<T, char>::value) return MPointerType::CHAR;
    if (std::is_same<T, std::string>::value) return MPointerType::STRING;
    return MPointerType::UNKNOWN;
}

template <typename T>
class MPointer {
private:
    int id_;
    static std::shared_ptr<MemoryManager::Stub> stub_;
    mutable T cached_value_;
    mutable bool dirty_;

    void fetchValue() const;
    void storeValue() const;
    void increaseRefCount();
    void decreaseRefCount();

public:
    static void Init(const std::string& server_address);
    static MPointer<T> New();

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

    int getId() const { return id_; }
};

// Declaración de especializaciones

template <>
void MPointer<std::string>::fetchValue() const;
template <>
void MPointer<std::string>::storeValue() const;


#endif // MPOINTERS_H