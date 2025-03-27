#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "generated/memory_manager.grpc.pb.h"
#include "MemoryManagerProgram.cpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
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

MemoryManagerProgram memManager(10);

class MemoryManagerServiceImpl final : public MemoryManager::Service {
public:
    Status Create(ServerContext* context, const CreateRequest* request, CreateResponse* response) override {
        std::cout << "[DEBUG] Inicio de Create - size: " << request->size()
                  << ", type: " << request->type() << std::endl;
        try {
            std::string typeStr;

            // Mapear el enum del request al tipo correspondiente
            switch(request->type()) {
                case CreateRequest::INT:
                    typeStr = "int";
                    break;
                case CreateRequest::FLOAT:
                    typeStr = "float";
                    break;
                case CreateRequest::DOUBLE:
                    typeStr = "double";
                    break;
                case CreateRequest::CHAR:
                    typeStr = "char";
                    break;
                case CreateRequest::STRING:
                    typeStr = "string";
                    break;
                default:
                    throw std::runtime_error("Tipo de dato no soportado");
            }

            int id = memManager.allocate(request->size(), typeStr);
            if (id == -1) {
                std::cerr << "[ERROR] Create - No se pudo asignar memoria" << std::endl;
                return Status(grpc::StatusCode::INTERNAL, "No se pudo asignar memoria");
            }

            std::cout << "[DEBUG] Create - Memoria asignada con ID: " << id
                      << " para tipo: " << typeStr << std::endl;
            response->set_id(id);
            return Status::OK;
        } catch (const std::exception& e) {
            std::cerr << "[EXCEPTION] Create - " << e.what() << std::endl;
            return Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }

    // Método Set mejorado
    Status Set(ServerContext* context, const SetRequest* request, SetResponse* response) override {
        try {
            auto id = request->id();
            std::string blockType = memManager.getBlockType(id);
            size_t blockSize = memManager.getBlockSize(id);

            // Validación básica
            if (request->value().empty()) {
                throw std::runtime_error("Empty value provided");
            }

            // Manejo específico por tipo
            if (blockType == "string") {
                if (request->value().size() > blockSize) {
                    throw std::runtime_error("String size exceeds block size");
                }
                memManager.setValue<std::string>(id, request->value());
            }
            else if (blockType == "int") {
                if (request->value().size() != sizeof(int)) {
                    throw std::runtime_error("Invalid size for int");
                }
                int value;
                memcpy(&value, request->value().data(), sizeof(int));
                memManager.setValue<int>(id, value);
            }
            else if (blockType == "float") {
                if (request->value().size() != sizeof(float)) {
                    throw std::runtime_error("Invalid size for float");
                }
                float value;
                memcpy(&value, request->value().data(), sizeof(float));
                memManager.setValue<float>(id, value);
            }
            else if (blockType == "double") {
                if (request->value().size() != sizeof(double)) {
                    throw std::runtime_error("Invalid size for double");
                }
                double value;
                memcpy(&value, request->value().data(), sizeof(double));
                memManager.setValue<double>(id, value);
            }
            else if (blockType == "char") {
                if (request->value().size() != sizeof(char)) {
                    throw std::runtime_error("Invalid size for char");
                }
                char value = request->value()[0];
                memManager.setValue<char>(id, value);
            }
            else {
                throw std::runtime_error("Unsupported block type: " + blockType);
            }

            response->set_success(true);
            return Status::OK;
        } catch (const std::exception& e) {
            response->set_success(false);
            response->set_error_message(e.what());
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }

    // Método Get mejorado
    Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) override {
        try {
            auto id = request->id();
            std::string blockType = memManager.getBlockType(id);
            void* blockAddr = memManager.getBlockAddress(id);
            size_t blockSize = memManager.getBlockSize(id);

            // Configurar el tipo en la respuesta
            if (blockType == "int") response->set_type(GetResponse::INT);
            else if (blockType == "float") response->set_type(GetResponse::FLOAT);
            else if (blockType == "double") response->set_type(GetResponse::DOUBLE);
            else if (blockType == "char") response->set_type(GetResponse::CHAR);
            else if (blockType == "string") response->set_type(GetResponse::STRING);
            else throw std::runtime_error("Unknown block type");

            // Manejo específico por tipo
            if (blockType == "string") {
                std::string value(static_cast<char*>(blockAddr), blockSize);
                response->set_value(value);
            }
            else if (blockType == "int") {
                int value = memManager.getValue<int>(id);
                response->set_value(std::string(reinterpret_cast<char*>(&value), sizeof(int)));
            }
            else if (blockType == "float") {
                float value = memManager.getValue<float>(id);
                response->set_value(std::string(reinterpret_cast<char*>(&value), sizeof(float)));
            }
            else if (blockType == "double") {
                double value = memManager.getValue<double>(id);
                response->set_value(std::string(reinterpret_cast<char*>(&value), sizeof(double)));
            }
            else if (blockType == "char") {
                char value = memManager.getValue<char>(id);
                response->set_value(std::string(1, value));
            }

            return Status::OK;
        } catch (const std::exception& e) {
            return Status(grpc::StatusCode::INTERNAL, std::string("Error retrieving value: ") + e.what());
        }
    }


    Status IncreaseRefCount(ServerContext* context, const RefCountRequest* request, RefCountResponse* response) override {
        std::cout << "[DEBUG] Inicio de IncreaseRefCount - id: " << request->id() << std::endl;
        try {
            int newCount = memManager.increaseRefCount(request->id());
            std::cout << "[DEBUG] IncreaseRefCount - Contador incrementado para id: "
                      << request->id() << " (nuevo valor: " << newCount << ")" << std::endl;
            response->set_success(true);
            response->set_ref_count(newCount);
            return Status::OK;
        } catch (const std::exception& e) {
            std::cerr << "[EXCEPTION] IncreaseRefCount - " << e.what() << std::endl;
            response->set_success(false);
            response->set_error_message(e.what());
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }

    Status DecreaseRefCount(ServerContext* context, const RefCountRequest* request, RefCountResponse* response) override {
        std::cout << "[DEBUG] Inicio de DecreaseRefCount - id: " << request->id() << std::endl;
        try {
            int newCount = memManager.decreaseRefCount(request->id());
            std::cout << "[DEBUG] DecreaseRefCount - Contador decrementado para id: "
                      << request->id() << " (nuevo valor: " << newCount << ")" << std::endl;
            response->set_success(true);
            response->set_ref_count(newCount);
            return Status::OK;
        } catch (const std::exception& e) {
            std::cerr << "[EXCEPTION] DecreaseRefCount - " << e.what() << std::endl;
            response->set_success(false);
            response->set_error_message(e.what());
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    MemoryManagerServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "[SERVER] Servidor gRPC escuchando en " << server_address << std::endl;
    server->Wait();
}

int main() {
    std::cout << "[SERVER] Iniciando servidor gRPC..." << std::endl;
    RunServer();
    return 0;
}
