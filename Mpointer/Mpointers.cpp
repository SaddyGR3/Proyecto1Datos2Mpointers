#include "MPointers.h"
#include <stdexcept>

template <typename T>
std::shared_ptr<MemoryManager::Stub> MPointer<T>::stub_ = nullptr;

template <typename T>
void MPointer<T>::Init(const std::string& server_address) {
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    stub_ = MemoryManager::NewStub(channel);

    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
    if (!channel->WaitForConnected(deadline)) {
        throw std::runtime_error("Failed to connect to MemoryManager");
    }
}

template <typename T>
MPointer<T> MPointer<T>::New() {
    ClientContext context;
    CreateRequest request;
    CreateResponse response;

    request.set_size(sizeof(T));

    if (std::is_same<T, int>::value) {
        request.set_type(CreateRequest::INT);
    } else if (std::is_same<T, float>::value) {
        request.set_type(CreateRequest::FLOAT);
    } else if (std::is_same<T, double>::value) {
        request.set_type(CreateRequest::DOUBLE);
    } else if (std::is_same<T, char>::value) {
        request.set_type(CreateRequest::CHAR);
    } else if (std::is_same<T, std::string>::value) {
        request.set_type(CreateRequest::STRING);
    } else {
        throw std::runtime_error("Unsupported type");
    }

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
void MPointer<T>::fetchValue() const {
    if (dirty_) return;

    ClientContext context;
    GetRequest request;
    GetResponse response;
    request.set_id(id_);

    Status status = stub_->Get(&context, request, &response);
    if (!status.ok()) {
        throw std::runtime_error("Failed to get value: " + status.error_message());
    }

    if constexpr (std::is_same_v<T, int>) {
        cached_value_ = response.int_value();
    } else if constexpr (std::is_same_v<T, float>) {
        cached_value_ = response.float_value();
    } else if constexpr (std::is_same_v<T, double>) {
        cached_value_ = response.double_value();
    } else if constexpr (std::is_same_v<T, char>) {
        cached_value_ = response.char_value();
    } else if constexpr (std::is_same_v<T, std::string>) {
        cached_value_ = response.string_value();
    } else {
        throw std::runtime_error("Unsupported type during fetchValue");
    }
}

template <typename T>
void MPointer<T>::storeValue() const {
    if (!dirty_) return;

    ClientContext context;
    SetRequest request;
    SetResponse response;
    request.set_id(id_);

    if constexpr (std::is_same_v<T, int>) {
        request.set_int_value(cached_value_);
    } else if constexpr (std::is_same_v<T, float>) {
        request.set_float_value(cached_value_);
    } else if constexpr (std::is_same_v<T, double>) {
        request.set_double_value(cached_value_);
    } else if constexpr (std::is_same_v<T, char>) {
        request.set_char_value(cached_value_);
    } else if constexpr (std::is_same_v<T, std::string>) {
        request.set_string_value(cached_value_);
    } else {
        throw std::runtime_error("Unsupported type during storeValue");
    }

    Status status = stub_->Set(&context, request, &response);
    if (!status.ok() || !response.success()) {
        throw std::runtime_error("Failed to set value: " + status.error_message());
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
        throw std::runtime_error("IncreaseRefCount failed");
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
        std::cerr << "Warning: DecreaseRefCount failed" << std::endl;
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
template class MPointer<int>;
template class MPointer<float>;
template class MPointer<double>;
template class MPointer<char>;
template class MPointer<std::string>;

