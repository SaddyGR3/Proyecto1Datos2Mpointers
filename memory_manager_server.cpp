#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "generated/memory_manager.grpc.pb.h"
#include "MemoryManagerProgram.cpp"  // Cambiado a .h

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
using memorymanager::DataType;

MemoryManagerProgram memManager(10);

// Función auxiliar para convertir DataType a string
std::string DataTypeToString(DataType type) {
    switch(type) {
        case DataType::INT: return "int";
        case DataType::FLOAT: return "float";
        case DataType::CHAR: return "char";
        case DataType::STRING: return "string";
        default: return "undefined";
    }
}

class MemoryManagerServiceImpl final : public MemoryManager::Service {
public:
    Status Create(ServerContext* context, const CreateRequest* request, CreateResponse* response) override {
        try {
            std::string typeStr;
            size_t size = 0;

            switch(request->type()) {
                case DataType::INT:
                    typeStr = "int";
                    size = sizeof(int32_t);
                    break;
                case DataType::FLOAT:
                    typeStr = "float";
                    size = sizeof(float);
                    break;
                case DataType::CHAR:
                    typeStr = "char";
                    size = sizeof(char);
                    break;
                case DataType::STRING:
                    typeStr = "string";
                    size = request->size() > 0 ? request->size() : 64; // Default 64 bytes para strings
                    break;
                default:
                    throw std::runtime_error("Tipo no soportado");
            }

            int id = memManager.allocate(size, typeStr);
            response->set_id(id);
            response->set_type(request->type());
            response->set_actual_size(size);
            return Status::OK;
        } catch (const std::exception& e) {
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }

    Status Set(ServerContext* context, const SetRequest* request, SetResponse* response) override {
        try {
            std::string blockType = memManager.getBlockType(request->id());

            // Validar tipo
            if ((request->type() == DataType::STRING && blockType != "string") ||
                (request->type() != DataType::STRING && blockType == "string")) {
                throw std::runtime_error("Type mismatch");
            }

            if (request->value_case() == SetRequest::kBinaryData) {
                // Manejar tipos binarios
                const auto& data = request->binary_data();
                size_t expected_size = 0;

                switch(request->type()) {
                    case DataType::INT:
                        expected_size = sizeof(int32_t);
                        if (data.size() != expected_size) throw std::runtime_error("Invalid int size");
                        memManager.setValue<int32_t>(request->id(), *reinterpret_cast<const int32_t*>(data.data()));
                        break;
                    case DataType::FLOAT:
                        expected_size = sizeof(float);
                        if (data.size() != expected_size) throw std::runtime_error("Invalid float size");
                        memManager.setValue<float>(request->id(), *reinterpret_cast<const float*>(data.data()));
                        break;
                    case DataType::CHAR:
                        expected_size = sizeof(char);
                        if (data.size() != expected_size) throw std::runtime_error("Invalid char size");
                        memManager.setValue<char>(request->id(), data[0]);
                        break;
                    default:
                        throw std::runtime_error("Invalid binary type");
                }
                response->set_bytes_written(expected_size);
            }
            else if (request->value_case() == SetRequest::kStrData) {
                // Manejar strings
                if (request->type() != DataType::STRING) {
                    throw std::runtime_error("Expected string type");
                }
                const auto& str = request->str_data();
                if (str.size() > memManager.getBlockSize(request->id())) {
                    throw std::runtime_error("String too large");
                }
                memManager.setValue<std::string>(request->id(), str);
                response->set_bytes_written(str.size());
            }
            else {
                throw std::runtime_error("No data provided");
            }

            response->set_success(true);
            return Status::OK;
        } catch (const std::exception& e) {
            response->set_success(false);
            response->set_error_message(e.what());
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }

    Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) override {
        try {
            std::string blockType = memManager.getBlockType(request->id());

            // Validar tipo esperado
            if ((request->expected_type() == DataType::STRING && blockType != "string") ||
                (request->expected_type() != DataType::STRING && blockType == "string")) {
                throw std::runtime_error("Type mismatch");
                }

            response->set_type(request->expected_type());

            if (blockType == "string") {
                std::string value = memManager.getValue<std::string>(request->id());
                // Forzar el string como datos binarios para evitar validación UTF-8
                response->set_binary_data(value);  // <-- Cambio clave aquí
            } else {
                // Manejar tipos binarios (existente)
                std::string binary_data;
                binary_data.resize(blockType == "int" ? sizeof(int32_t) :
                                 blockType == "float" ? sizeof(float) : sizeof(char));

                if (blockType == "int") {
                    int32_t value = memManager.getValue<int32_t>(request->id());
                    memcpy(binary_data.data(), &value, sizeof(int32_t));
                }
                else if (blockType == "float") {
                    float value = memManager.getValue<float>(request->id());
                    memcpy(binary_data.data(), &value, sizeof(float));
                }
                else if (blockType == "char") {
                    char value = memManager.getValue<char>(request->id());
                    binary_data[0] = value;
                }
                response->set_binary_data(binary_data);
            }

            return Status::OK;
        } catch (const std::exception& e) {
            return Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }

    Status IncreaseRefCount(ServerContext* context, const RefCountRequest* request, RefCountResponse* response) override {
        try {
            int id = request->id();
            int newCount = memManager.increaseRefCount(id);

            response->set_ref_count(newCount);
            return Status::OK;
        } catch (const std::exception& e) {
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }

    Status DecreaseRefCount(ServerContext* context, const RefCountRequest* request, RefCountResponse* response) override {
        try {
            int id = request->id();
            int newCount = memManager.decreaseRefCount(id);

            response->set_ref_count(newCount);
            return Status::OK;
        } catch (const std::exception& e) {
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
