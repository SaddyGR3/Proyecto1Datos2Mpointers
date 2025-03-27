#include <iostream>
#include <memory>
#include <string>
#include "memory_manager.grpc.pb.h"

#include <grpcpp/grpcpp.h>


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

class MemoryManagerClient {
public:
    MemoryManagerClient(std::shared_ptr<Channel> channel) : stub_(MemoryManager::NewStub(channel)) {}

    int Create(size_t size, const std::string& type) {
        CreateRequest request;
        request.set_size(size);  // Asegúrate de que el tamaño sea correcto
        request.set_type(type);  // Asegúrate de que el tipo sea correcto

        CreateResponse response;
        ClientContext context;

        Status status = stub_->Create(&context, request, &response);
        if (status.ok()) {
            return response.id();
        } else {
            std::cerr << "Create RPC failed: " << status.error_message() << std::endl;
            return -1;
        }
    }

    bool Set(int id, const std::string& value) {
        SetRequest request;
        request.set_id(id);
        request.set_value(value);  // Envía el valor como bytes

        SetResponse response;
        ClientContext context;

        Status status = stub_->Set(&context, request, &response);

        if (!status.ok()) {
            std::cerr << "Error en la llamada gRPC: " << status.error_message() << std::endl;
            return false;
        }

        if (!response.success()) {
            std::cerr << "El servidor no pudo procesar la solicitud." << std::endl;
            return false;
        }

        return true;
    }

    std::string Get(int id) {
        GetRequest request;
        request.set_id(id);

        GetResponse response;
        ClientContext context;

        Status status = stub_->Get(&context, request, &response);
        if (status.ok()) {
            return response.value();  // Asegúrate de que esto sea compatible con `bytes` en el .proto
        } else {
            std::cerr << "Get RPC failed: " << status.error_message() << std::endl;
            return "";
        }
    }

private:
    std::unique_ptr<MemoryManager::Stub> stub_;
};

int main() {
    MemoryManagerClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    try {
        int id = client.Create(4, "char");  // Asignar un bloque de 1 byte
        if (id == -1) {
            std::cerr << "Error al asignar memoria en el servidor." << std::endl;
            return 1;
        }

        std::cout << "Bloque asignado con ID: " << id << std::endl;

        client.Set(id, "ABc");  // Escribir el valor "A" en el bloque

        std::string value = client.Get(id); // Leer el valor
        std::cout << "Valor leído desde el servidor: " << value << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}



