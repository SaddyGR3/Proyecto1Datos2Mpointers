#include "MPointers.h"
#include <stdexcept>
#include <chrono>

// Definir el stub compartido
std::shared_ptr<MemoryManager::Stub> MPointerBase::stub_ = nullptr;

// Implementar Init una sola vez
void MPointerBase::Init(const std::string& server_address) {
    if (stub_ != nullptr) return;  // Ya está inicializado

    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    stub_ = MemoryManager::NewStub(channel);

    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
    if (!channel->WaitForConnected(deadline)) {
        throw std::runtime_error("Failed to connect to MemoryManager");
    }
}

template <typename T>
MPointer<T> MPointer<T>::New(size_t size) {
    ClientContext context;
    CreateRequest request;
    CreateResponse response;

    uint32_t request_size = static_cast<uint32_t>(size > 0 ? size : getTypeSize());
    request.set_size(request_size);
    request.set_type(getProtoType());

    Status status = stub_->Create(&context, request, &response);
    if (!status.ok()) {
        throw std::runtime_error("Create failed: " + status.error_message());
    }

    return MPointer<T>(response.id());
}

template <typename T>
MPointer<T>::MPointer(int id) : id_(id), cached_value_(), dirty_(false) {
    if (id_ != -1) increaseRefCount();
}

template <typename T>
MPointer<T>::MPointer(const MPointer& other) :
    id_(other.id_), cached_value_(other.cached_value_), dirty_(other.dirty_) {
    if (id_ != -1) increaseRefCount();
}

template <typename T>
MPointer<T>::MPointer(MPointer&& other) noexcept :
    id_(other.id_), cached_value_(std::move(other.cached_value_)), dirty_(other.dirty_) {
    other.id_ = -1;
    other.dirty_ = false;
}

template <typename T>
MPointer<T>::~MPointer() {
    if (id_ != -1) decreaseRefCount();
}

template <typename T>
void MPointer<T>::fetchValue() const {
    if (dirty_) return;

    ClientContext context;
    GetRequest request;
    GetResponse response;
    request.set_id(id_);
    request.set_expected_type(getProtoType());

    Status status = stub_->Get(&context, request, &response);
    if (!status.ok()) {
        throw std::runtime_error("Failed to get value: " + status.error_message());
    }

    if constexpr (std::is_same_v<T, std::string>) {
        if (response.value_case() != GetResponse::kStrData) {
            throw std::runtime_error("Expected string value not received");
        }
        cached_value_ = response.str_data();
    } else {
        if (response.value_case() != GetResponse::kBinaryData) {
            throw std::runtime_error("Expected binary data not received");
        }
        if (response.binary_data().size() != sizeof(T)) {
            throw std::runtime_error("Invalid data size received");
        }
        // Copia segura del valor binario
        memcpy(&cached_value_, response.binary_data().data(), sizeof(T));
    }
}

template <typename T>
void MPointer<T>::storeValue() const {
    if (!dirty_) return;

    ClientContext context;
    SetRequest request;
    SetResponse response;
    request.set_id(id_);
    request.set_type(getProtoType());

    if constexpr (std::is_same_v<T, std::string>) {
        request.set_str_data(cached_value_);
    } else {
        request.set_binary_data(std::string(reinterpret_cast<const char*>(&cached_value_), sizeof(T)));
    }

    Status status = stub_->Set(&context, request, &response);
    if (!status.ok() || !response.success()) {
        throw std::runtime_error("Failed to set value: " +
            (status.ok() ? response.error_message() : status.error_message()));
    }
    dirty_ = false;
}

template <>
void MPointer<std::string>::fetchValue() const {
    if (dirty_) return;

    ClientContext context;
    GetRequest request;
    GetResponse response;
    request.set_id(id_);
    request.set_expected_type(DataType::STRING);

    Status status = stub_->Get(&context, request, &response);
    if (!status.ok()) {
        throw std::runtime_error("Failed to get string value: " + status.error_message());
    }

    // Cambio clave: usar sólo hasta el null terminator
    cached_value_ = response.binary_data(); // o str_data según tu proto
    cached_value_ = cached_value_.c_str(); // Esto trunca en el primer null byte
    dirty_ = false;
}

template <>
void MPointer<std::string>::storeValue() const {
    if (!dirty_) return;

    ClientContext context;
    SetRequest request;
    SetResponse response;
    request.set_id(id_);
    request.set_type(DataType::STRING);

    // Asegurar que el string sea UTF-8 válido
    std::string utf8_valid_str;
    for (char c : cached_value_) {
        if ((c & 0xC0) != 0x80) { // Validación básica UTF-8
            utf8_valid_str += c;
        }
    }
    request.set_str_data(utf8_valid_str);

    Status status = stub_->Set(&context, request, &response);
    if (!status.ok() || !response.success()) {
        throw std::runtime_error("Failed to set string value: " +
            (status.ok() ? response.error_message() : status.error_message()));
    }
    dirty_ = false;
}

template <typename T>
void MPointer<T>::increaseRefCount() {
    ClientContext context;
    RefCountRequest request;
    RefCountResponse response;
    request.set_id(id_);

    Status status = stub_->IncreaseRefCount(&context, request, &response);
    if (!status.ok()) {
        throw std::runtime_error("IncreaseRefCount failed: " + status.error_message());
    }
}

template <typename T>
void MPointer<T>::decreaseRefCount() {
    ClientContext context;
    RefCountRequest request;
    RefCountResponse response;
    request.set_id(id_);

    Status status = stub_->DecreaseRefCount(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Warning: DecreaseRefCount failed: " << status.error_message() << std::endl;
    }
}

template <typename T>
MPointer<T>& MPointer<T>::operator=(const MPointer<T>& other) {
    if (this != &other) {
        if (id_ != -1) decreaseRefCount();
        id_ = other.id_;
        cached_value_ = other.cached_value_;
        dirty_ = true;
        if (id_ != -1) increaseRefCount();
    }
    return *this;
}

template <typename T>
MPointer<T>& MPointer<T>::operator=(MPointer<T>&& other) noexcept {
    if (this != &other) {
        if (id_ != -1) decreaseRefCount();
        id_ = other.id_;
        cached_value_ = std::move(other.cached_value_);
        dirty_ = other.dirty_;
        other.id_ = -1;
        other.dirty_ = false;
    }
    return *this;
}

template <typename T>
MPointer<T>& MPointer<T>::operator=(const T& value) {
    if (id_ == -1) throw std::runtime_error("Assigning to null MPointer");
    cached_value_ = value;
    dirty_ = true;
    storeValue();
    return *this;
}

template <typename T>
T& MPointer<T>::operator*() {
    if (id_ == -1) throw std::runtime_error("Dereferencing null MPointer");
    fetchValue();
    dirty_ = true;
    return cached_value_;
}

template <typename T>
const T& MPointer<T>::operator*() const {
    if (id_ == -1) throw std::runtime_error("Dereferencing null MPointer");
    fetchValue();
    return cached_value_;
}

template <typename T>
T* MPointer<T>::operator->() {
    return &(this->operator*());
}

template <typename T>
const T* MPointer<T>::operator->() const {
    return &(this->operator*());
}

// Instanciaciones explícitas
template class MPointer<int32_t>;
template class MPointer<float>;
template class MPointer<char>;
template class MPointer<std::string>;


