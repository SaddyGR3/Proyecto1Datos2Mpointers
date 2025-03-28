#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "generated/memory_manager.grpc.pb.h"
#include "MemoryManagerProgram.cpp"

namespace fs = std::filesystem;

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

class MemoryManagerServiceImpl final : public MemoryManager::Service {
private:
    MemoryManagerProgram& memManager;
    const std::string& dumpFolder;

    // Función helper para imprimir logs (sin fecha/hora)
    void logOperation(const std::string& operation, const std::string& details) {
        std::cout << operation << " - " << details << std::endl;
    }

public:
    explicit MemoryManagerServiceImpl(MemoryManagerProgram& manager, const std::string& folder)
        : memManager(manager), dumpFolder(folder) {}

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
                    size = request->size() > 0 ? request->size() : 64;
                    break;
                default:
                    throw std::runtime_error("Unsupported type");
            }

            int id = memManager.allocate(size, typeStr);
            response->set_id(id);
            response->set_type(request->type());
            response->set_actual_size(size);

            logOperation("NEW BLOCK",
                "ID: " + std::to_string(id) +
                " | Type: " + typeStr +
                " | Size: " + std::to_string(size) + " bytes");

            return Status::OK;
        } catch (const std::exception& e) {
            logOperation("ERROR", std::string("Create failed: ") + e.what());
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }

    Status Set(ServerContext* context, const SetRequest* request, SetResponse* response) override {
        try {
            std::string blockType = memManager.getBlockType(request->id());

            if ((request->type() == DataType::STRING && blockType != "string") ||
                (request->type() != DataType::STRING && blockType == "string")) {
                throw std::runtime_error("Type mismatch");
            }

            if (request->value_case() == SetRequest::kBinaryData) {
                const auto& data = request->binary_data();
                size_t expected_size = 0;

                switch(request->type()) {
                    case DataType::INT: {
                        expected_size = sizeof(int32_t);
                        if (data.size() != expected_size) throw std::runtime_error("Invalid int size");
                        int32_t value = *reinterpret_cast<const int32_t*>(data.data());
                        memManager.setValue<int32_t>(request->id(), value);
                        logOperation("SET VALUE",
                            "ID: " + std::to_string(request->id()) +
                            " | Value: " + std::to_string(value) +
                            " | Type: int");
                        break;
                    }
                    case DataType::FLOAT: {
                        expected_size = sizeof(float);
                        if (data.size() != expected_size) throw std::runtime_error("Invalid float size");
                        float value = *reinterpret_cast<const float*>(data.data());
                        memManager.setValue<float>(request->id(), value);
                        logOperation("SET VALUE",
                            "ID: " + std::to_string(request->id()) +
                            " | Value: " + std::to_string(value) +
                            " | Type: float");
                        break;
                    }
                    case DataType::CHAR: {
                        expected_size = sizeof(char);
                        if (data.size() != expected_size) throw std::runtime_error("Invalid char size");
                        char value = data[0];
                        memManager.setValue<char>(request->id(), value);
                        logOperation("SET VALUE",
                            "ID: " + std::to_string(request->id()) +
                            " | Value: '" + std::string(1, value) + "'" +
                            " | Type: char");
                        break;
                    }
                    default:
                        throw std::runtime_error("Invalid binary type");
                }
                response->set_bytes_written(expected_size);
            }
            else if (request->value_case() == SetRequest::kStrData) {
                if (request->type() != DataType::STRING) {
                    throw std::runtime_error("Expected string type");
                }
                const auto& str = request->str_data();
                if (str.size() > memManager.getBlockSize(request->id())) {
                    throw std::runtime_error("String too large");
                }
                memManager.setValue<std::string>(request->id(), str);
                logOperation("SET VALUE",
                    "ID: " + std::to_string(request->id()) +
                    " | Value: \"" + str + "\"" +
                    " | Type: string");
                response->set_bytes_written(str.size());
            }
            else {
                throw std::runtime_error("No data provided");
            }

            response->set_success(true);
            return Status::OK;
        } catch (const std::exception& e) {
            logOperation("ERROR", std::string("Set failed: ") + e.what());
            response->set_success(false);
            response->set_error_message(e.what());
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }

    Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) override {
        try {
            std::string blockType = memManager.getBlockType(request->id());

            if ((request->expected_type() == DataType::STRING && blockType != "string") ||
                (request->expected_type() != DataType::STRING && blockType == "string")) {
                throw std::runtime_error("Type mismatch");
            }

            response->set_type(request->expected_type());

            if (blockType == "string") {
                std::string value = memManager.getValue<std::string>(request->id());
                response->set_binary_data(value);
                logOperation("GET VALUE",
                    "ID: " + std::to_string(request->id()) +
                    " | Value: \"" + value + "\"" +
                    " | Type: string");
            } else {
                std::string binary_data;
                binary_data.resize(blockType == "int" ? sizeof(int32_t) :
                                 blockType == "float" ? sizeof(float) : sizeof(char));

                if (blockType == "int") {
                    int32_t value = memManager.getValue<int32_t>(request->id());
                    memcpy(binary_data.data(), &value, sizeof(int32_t));
                    logOperation("GET VALUE",
                        "ID: " + std::to_string(request->id()) +
                        " | Value: " + std::to_string(value) +
                        " | Type: int");
                }
                else if (blockType == "float") {
                    float value = memManager.getValue<float>(request->id());
                    memcpy(binary_data.data(), &value, sizeof(float));
                    logOperation("GET VALUE",
                        "ID: " + std::to_string(request->id()) +
                        " | Value: " + std::to_string(value) +
                        " | Type: float");
                }
                else if (blockType == "char") {
                    char value = memManager.getValue<char>(request->id());
                    binary_data[0] = value;
                    logOperation("GET VALUE",
                        "ID: " + std::to_string(request->id()) +
                        " | Value: '" + std::string(1, value) + "'" +
                        " | Type: char");
                }
                response->set_binary_data(binary_data);
            }

            return Status::OK;
        } catch (const std::exception& e) {
            logOperation("ERROR", std::string("Get failed: ") + e.what());
            return Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }

    Status IncreaseRefCount(ServerContext* context, const RefCountRequest* request, RefCountResponse* response) override {
        try {
            int id = request->id();
            int newCount = memManager.increaseRefCount(id);
            response->set_ref_count(newCount);
            logOperation("REF COUNT",
                "ID: " + std::to_string(id) +
                " | New count: " + std::to_string(newCount) +
                " (Increased)");
            return Status::OK;
        } catch (const std::exception& e) {
            logOperation("ERROR", std::string("Increase ref count failed: ") + e.what());
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }

    Status DecreaseRefCount(ServerContext* context, const RefCountRequest* request, RefCountResponse* response) override {
        try {
            int id = request->id();
            int newCount = memManager.decreaseRefCount(id);
            response->set_ref_count(newCount);
            logOperation("REF COUNT",
                "ID: " + std::to_string(id) +
                " | New count: " + std::to_string(newCount) +
                " (Decreased)");
            return Status::OK;
        } catch (const std::exception& e) {
            logOperation("ERROR", std::string("Decrease ref count failed: ") + e.what());
            return Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
        }
    }
};

void RunServer(int port, size_t memSizeMB, const std::string& dumpFolder) {
    std::string server_address = "0.0.0.0:" + std::to_string(port);
    MemoryManagerProgram memManager(memSizeMB);  //(memSizeMB, dumpFolder); cuando se implemente el dump
    MemoryManagerServiceImpl service(memManager, dumpFolder);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "SERVIDOR EN LINEA - ESCUCHANDO EN " << server_address << std::endl;
    std::cout << "CONFIG - Memory: " << memSizeMB << " MB | Dump folder: " << dumpFolder << std::endl;

    server->Wait();
}

void mostrarUso() {
    std::cerr << "Uso: ./mem-mgr –port LISTEN_PORT –memsize SIZE_MB –dumpFolder DUMP_FOLDER\n"
              << "Ejemplo: ./mem-mgr –port 50051 –memsize 10 –dumpFolder ./dumps\n";
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    int port = 50051;
    size_t memSizeMB = 10;
    std::string dumpFolder = "./dumps";

    // Parsear argumentos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-port" || arg == "--port") {
            if (i + 1 < argc) port = std::stoi(argv[++i]);
            else mostrarUso();
        }
        else if (arg == "-memsize" || arg == "--memsize") {
            if (i + 1 < argc) memSizeMB = std::stoul(argv[++i]);
            else mostrarUso();
        }
        else if (arg == "-dumpFolder" || arg == "--dumpFolder") {
            if (i + 1 < argc) {
                dumpFolder = argv[++i];
                if (!fs::exists(dumpFolder)) {
                    fs::create_directories(dumpFolder);
                }
            } else mostrarUso();
        }
        else mostrarUso();
    }

    std::cout << "Iniciando Servidor...\n";
    RunServer(port, memSizeMB, dumpFolder);
    return 0;
}
